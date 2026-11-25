#include "Toast.h"

#include "../Common/PhostideCommon.h"

#include <mutex>

#include <unknwn.h>
#include <NotificationActivationCallback.h>
#include <propkey.h>
#include <propvarutil.h>
#include <shlobj.h>

#pragma comment(lib, "propsys.lib")

#include <winrt/Windows.Data.Xml.Dom.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Notifications.h>

#pragma comment(lib, "windowsapp.lib")

namespace phostide {
namespace {

namespace wun = winrt::Windows::UI::Notifications;
namespace wdxd = winrt::Windows::Data::Xml::Dom;

HWND g_window = nullptr;
UINT g_message = 0;

// Shown toasts must stay alive for their Activated handler to fire.
std::mutex g_liveMutex;
std::vector<wun::ToastNotification> g_live;
constexpr size_t kMaxLive = 8;

void PostActivation(const std::wstring& argument) {
  if (!g_window || !g_message) return;
  auto* arg = new std::wstring(argument);
  if (!PostMessageW(g_window, g_message, 0, reinterpret_cast<LPARAM>(arg))) delete arg;
}

std::wstring StartMenuShortcutPath() {
  PWSTR programs = nullptr;
  std::wstring lnk;
  if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programs)) && programs) {
    lnk = std::wstring(programs) + L"\\Phostide\\Phostide.lnk";
  }
  CoTaskMemFree(programs);
  return lnk;
}

// Shortcuts from Setup before 0.1.42 have no activator: then the in-process
// Activated event is the only way a (popup) click reaches us.
bool ShortcutHasActivator() {
  static const bool has = [] {
    const std::wstring lnk = StartMenuShortcutPath();
    if (lnk.empty()) return false;
    IPropertyStore* store = nullptr;
    if (FAILED(SHGetPropertyStoreFromParsingName(lnk.c_str(), nullptr, GPS_DEFAULT,
                                                 IID_PPV_ARGS(&store)))) {
      return false;
    }
    PROPVARIANT pv;
    PropVariantInit(&pv);
    GUID clsid{};
    const bool ok = SUCCEEDED(store->GetValue(PKEY_AppUserModel_ToastActivatorCLSID, &pv)) &&
                    SUCCEEDED(PropVariantToGUID(pv, &clsid)) &&
                    IsEqualGUID(clsid, kToastActivatorClsid);
    PropVariantClear(&pv);
    store->Release();
    return ok;
  }();
  return has;
}

struct ToastActivator : winrt::implements<ToastActivator, INotificationActivationCallback> {
  // invokedArgs: the clicked button's arguments, or the toast's launch argument.
  HRESULT __stdcall Activate(LPCWSTR, LPCWSTR invokedArgs, const NOTIFICATION_USER_INPUT_DATA*,
                             ULONG) noexcept override {
    PostActivation(invokedArgs ? invokedArgs : L"");
    return S_OK;
  }
};

struct ToastActivatorFactory : winrt::implements<ToastActivatorFactory, IClassFactory> {
  HRESULT __stdcall CreateInstance(IUnknown* outer, REFIID iid, void** result) noexcept override {
    if (!result) return E_POINTER;
    *result = nullptr;
    if (outer) return CLASS_E_NOAGGREGATION;
    try {
      return winrt::make_self<ToastActivator>()->QueryInterface(iid, result);
    } catch (...) {
      return E_OUTOFMEMORY;
    }
  }
  HRESULT __stdcall LockServer(BOOL) noexcept override { return S_OK; }
};

DWORD g_activatorCookie = 0;

}  // namespace

std::wstring XmlEscape(std::wstring_view text) {
  std::wstring out;
  out.reserve(text.size());
  for (const wchar_t c : text) {
    switch (c) {
      case L'&': out += L"&amp;"; break;
      case L'<': out += L"&lt;"; break;
      case L'>': out += L"&gt;"; break;
      case L'"': out += L"&quot;"; break;
      case L'\'': out += L"&apos;"; break;
      default: out += c; break;
    }
  }
  return out;
}

std::wstring BuildToastXml(const ToastContent& content) {
  std::wstring xml = L"<toast";
  if (!content.launchArgument.empty()) {
    xml += L" launch=\"" + XmlEscape(content.launchArgument) + L"\" activationType=\"foreground\"";
  }
  xml += L"><visual><binding template=\"ToastGeneric\"><text>" + XmlEscape(content.title) +
         L"</text><text>" + XmlEscape(content.body) + L"</text></binding></visual>";
  if (!content.buttons.empty()) {
    xml += L"<actions>";
    for (const auto& b : content.buttons) {
      xml += L"<action content=\"" + XmlEscape(b.label) + L"\" arguments=\"" +
             XmlEscape(b.argument) + L"\" activationType=\"foreground\"/>";
    }
    xml += L"</actions>";
  }
  // Notifications from a tray app should not chime.
  xml += L"<audio silent=\"true\"/></toast>";
  return xml;
}

bool ToastsAvailable() {
  static const bool available = [] {
    // Setup's per-user shortcut ({group}\Phostide.lnk) sets AppUserModelID
    // Doticca.Phostide (installer/Phostide.iss).
    const std::wstring lnk = StartMenuShortcutPath();
    return !lnk.empty() && GetFileAttributesW(lnk.c_str()) != INVALID_FILE_ATTRIBUTES;
  }();
  return available;
}

void ToastInit(HWND window, UINT message) {
  g_window = window;
  g_message = message;
  if (!ToastsAvailable() || g_activatorCookie) return;
  // Also answers a click that started us (LocalServer32): COM waits for this.
  try {
    auto factory = winrt::make_self<ToastActivatorFactory>();
    CoRegisterClassObject(kToastActivatorClsid, factory.get(), CLSCTX_LOCAL_SERVER,
                          REGCLS_MULTIPLEUSE, &g_activatorCookie);
  } catch (...) {
  }
}

bool ShowToast(const ToastContent& content) {
  if (!ToastsAvailable()) return false;
  try {
    wdxd::XmlDocument doc;
    doc.LoadXml(BuildToastXml(content));
    wun::ToastNotification toast(doc);
    if (!content.tag.empty()) toast.Tag(content.tag);
    if (content.expiryMinutes > 0) {
      toast.ExpirationTime(winrt::clock::now() + std::chrono::minutes(content.expiryMinutes));
    }
    toast.SuppressPopup(content.silentToCenter);
    if (!ShortcutHasActivator()) {
      // Old shortcut: Windows reports popup clicks only through this event (the
      // activator would deliver them a second time).
      toast.Activated([](wun::ToastNotification const&,
                         winrt::Windows::Foundation::IInspectable const& args) {
        if (auto a = args.try_as<wun::ToastActivatedEventArgs>()) {
          PostActivation(std::wstring(a.Arguments()));
        }
      });
    }
    auto notifier = wun::ToastNotificationManager::CreateToastNotifier(kAppUserModelId.data());
    if (notifier.Setting() != wun::NotificationSetting::Enabled) {
      // Turned off for Phostide (or by policy): respect it, no balloon either.
      return true;
    }
    notifier.Show(toast);
    std::lock_guard lock(g_liveMutex);
    g_live.push_back(toast);
    if (g_live.size() > kMaxLive) g_live.erase(g_live.begin());
    return true;
  } catch (...) {
    return false;
  }
}

void ToastShutdown() {
  if (g_activatorCookie) {
    CoRevokeClassObject(g_activatorCookie);
    g_activatorCookie = 0;
  }
  if (!ToastsAvailable()) return;
  try {
    wun::ToastNotificationManager::History().Clear(kAppUserModelId.data());
  } catch (...) {
  }
  std::lock_guard lock(g_liveMutex);
  g_live.clear();
}

}  // namespace phostide
