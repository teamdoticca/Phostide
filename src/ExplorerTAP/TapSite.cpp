#include "TapSite.h"

#include "VisualTreeWatcher.h"
#include "../Common/PhostideCommon.h"

#include <atomic>
#include <string>
#include <thread>

#include <xamlOM.h>

namespace phostide {
namespace {

using PFN_InitializeXamlDiagnosticsEx = HRESULT(WINAPI*)(LPCWSTR, DWORD, LPCWSTR, LPCWSTR, CLSID,
                                                          LPCWSTR);

std::wstring ModulePath() {
  HMODULE module = nullptr;
  if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                          reinterpret_cast<LPCWSTR>(&ModulePath), &module) ||
      !module) {
    return {};
  }
  wchar_t path[MAX_PATH]{};
  const DWORD n = GetModuleFileNameW(module, path, MAX_PATH);
  return n ? std::wstring(path, n) : std::wstring{};
}

}  // namespace

winrt::weak_ref<VisualTreeWatcher> TAPSite::s_watcher_{};

HANDLE TAPSite::ReadyEvent() {
  return CreateEventW(nullptr, TRUE, FALSE, kTapReadyEvent.data());
}

void TAPSite::RequestRescan() {
  if (auto watcher = s_watcher_.get()) {
    try {
      watcher->ScanExistingViaHitTest();
    } catch (...) {
    }
  }
}

DWORD WINAPI TAPSite::Install(void*) {
  HANDLE ready = ReadyEvent();
  const auto signalReady = [&] {
    if (ready) SetEvent(ready);
  };
  const auto log = [](const std::wstring& line) {
    wchar_t dir[MAX_PATH]{};
    GetTempPathW(MAX_PATH, dir);
    wcscat_s(dir, L"Phostide");
    CreateDirectoryW(dir, nullptr);
    wchar_t path[MAX_PATH]{};
    swprintf_s(path, L"%s\\tap.log", dir);
    HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL, nullptr);
    if (f == INVALID_HANDLE_VALUE) return;
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t buf[768];
    swprintf_s(buf, L"%02u:%02u:%02u %s\r\n", st.wHour, st.wMinute, st.wSecond, line.c_str());
    DWORD written = 0;
    WriteFile(f, buf, static_cast<DWORD>(wcslen(buf) * sizeof(wchar_t)), &written, nullptr);
    CloseHandle(f);
  };

  const std::wstring location = ModulePath();
  log(L"Install begin path=" + location);
  if (location.empty()) {
    signalReady();
    return static_cast<DWORD>(E_FAIL);
  }

  // Brief wait for Shell_TrayWnd; XAML island may still be warming (ixde retries below).
  for (int i = 0; i < 40 && !FindWindowW(L"Shell_TrayWnd", nullptr); ++i) {
    Sleep(250);
  }

  HMODULE wux = LoadLibraryExW(L"Windows.UI.Xaml.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (!wux) {
    log(L"Windows.UI.Xaml.dll load failed");
    signalReady();
    return GetLastError();
  }

  auto ixde = reinterpret_cast<PFN_InitializeXamlDiagnosticsEx>(
      GetProcAddress(wux, "InitializeXamlDiagnosticsEx"));
  if (!ixde) {
    log(L"InitializeXamlDiagnosticsEx missing");
    signalReady();
    return GetLastError();
  }

  // Match TranslucentTB TAPSite::Install: one ixde call per fresh thread, no CoInit,
  // VisualDiagConnection1..60. Multiple calls / CoUninitialize on the same thread can
  // leave diagnostics stuck returning ERROR_NOT_FOUND for the process lifetime.
  const DWORD pid = GetCurrentProcessId();
  HRESULT hr = E_FAIL;
  for (uint8_t attempt = 1; attempt <= 60; ++attempt) {
    const std::wstring conn = L"VisualDiagConnection" + std::to_wstring(attempt);
    std::thread([&hr, ixde, pid, &location, conn] {
      hr = ixde(conn.c_str(), pid, nullptr, location.c_str(), kTapSiteClsid, nullptr);
    }).join();
    if (SUCCEEDED(hr)) {
      log(L"ixde succeeded attempt=" + std::to_wstring(attempt));
      // Ready event is signaled by VisualTreeWatcher after AdviseVisualTreeChange.
      return S_OK;
    }
    if (attempt == 1 || attempt % 5 == 0) {
      wchar_t msg[128];
      swprintf_s(msg, L"ixde hr=0x%08X attempt=%u", static_cast<unsigned>(hr), attempt);
      log(msg);
    }
    Sleep(500);
  }

  log(L"ixde gave up");
  signalReady();
  return static_cast<DWORD>(hr);
}

HRESULT STDMETHODCALLTYPE TAPSite::SetSite(IUnknown* pUnkSite) try {
  if (s_watcher_.get()) {
    return E_ILLEGAL_METHOD_CALL;
  }
  site_.copy_from(pUnkSite);
  if (site_) {
    HANDLE ready = ReadyEvent();
    s_watcher_ = winrt::make_self<VisualTreeWatcher>(site_, ready);
  }
  return S_OK;
} catch (...) {
  return winrt::to_hresult();
}

HRESULT STDMETHODCALLTYPE TAPSite::GetSite(REFIID riid, void** ppvSite) noexcept {
  if (!ppvSite) return E_POINTER;
  *ppvSite = nullptr;
  return site_ ? site_->QueryInterface(riid, ppvSite) : E_NOINTERFACE;
}

}  // namespace phostide
