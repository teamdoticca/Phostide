#include "VisualTreeWatcher.h"

#include "TaskbarAppearanceService.h"
#include "../Common/PhostideCommon.h"

#include <functional>
#include <string>
#include <thread>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.Foundation.h>
#pragma pop_macro("GetCurrentTime")

namespace wux = winrt::Windows::UI::Xaml;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxs = winrt::Windows::UI::Xaml::Shapes;
namespace wucore = winrt::Windows::UI::Core;

namespace phostide {
namespace {

bool TypeIsRectangle(std::wstring_view type) {
  return type.find(L"Shapes.Rectangle") != std::wstring_view::npos;
}

bool NameIsBackgroundFill(std::wstring_view name) {
  return name == L"BackgroundFill";
}

void HitTestWindow(IXamlDiagnostics* diag, HWND hwnd,
                   const std::function<void(InstanceHandle)>& onHandle) {
  if (!diag || !hwnd || !IsWindow(hwnd)) return;
  RECT rc{};
  if (!GetWindowRect(hwnd, &rc)) return;
  rc.top += 2;
  rc.bottom -= 2;
  rc.left += 2;
  rc.right -= 2;

  UINT count = 0;
  InstanceHandle* handles = nullptr;
  if (FAILED(diag->HitTest(rc, &count, &handles)) || !handles) return;
  for (UINT i = 0; i < count; ++i) {
    onHandle(handles[i]);
  }
  CoTaskMemFree(handles);
}

}  // namespace

void VisualTreeWatcher::Log(const wchar_t* msg) {
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
  wchar_t line[512];
  swprintf_s(line, L"%02u:%02u:%02u %s\r\n", st.wHour, st.wMinute, st.wSecond, msg);
  DWORD written = 0;
  WriteFile(f, line, static_cast<DWORD>(wcslen(line) * sizeof(wchar_t)), &written, nullptr);
  CloseHandle(f);
}

VisualTreeWatcher::VisualTreeWatcher(winrt::com_ptr<IUnknown> site, HANDLE readyEvent)
    : xamlDiagnostics_(site.as<IXamlDiagnostics>()), readyEvent_(readyEvent) {
  std::thread([self = get_strong()] {
    try {
      winrt::check_hresult(
          self->xamlDiagnostics_.as<IVisualTreeService>()->AdviseVisualTreeChange(self.get()));
      self->Log(L"AdviseVisualTreeChange ok");

      winrt::Windows::Foundation::IInspectable dispatcherObj{nullptr};
      if (SUCCEEDED(self->xamlDiagnostics_->GetDispatcher(
              reinterpret_cast<::IInspectable**>(winrt::put_abi(dispatcherObj)))) &&
          dispatcherObj) {
        if (auto dispatcher = dispatcherObj.try_as<wucore::CoreDispatcher>()) {
          dispatcher
              .RunAsync(wucore::CoreDispatcherPriority::Normal,
                        winrt::Windows::UI::Core::DispatchedHandler([self] {
                          self->ScanExistingViaHitTest();
                        }))
              .get();
        } else {
          self->Log(L"Dispatcher is not CoreDispatcher; scanning inline");
          self->ScanExistingViaHitTest();
        }
      } else {
        self->Log(L"GetDispatcher failed; scanning inline");
        self->ScanExistingViaHitTest();
      }
    } catch (...) {
      self->Log(L"AdviseVisualTreeChange failed");
    }
    if (self->readyEvent_) {
      SetEvent(self->readyEvent_);
    }
  }).detach();
}

void VisualTreeWatcher::TryRegisterShape(wuxs::Shape const& shape) {
  if (!shape) return;
  try {
    if (shape.Name() != L"BackgroundFill") return;
  } catch (...) {
    return;
  }
  Log(L"Registering BackgroundFill shape");
  TaskbarAppearanceService::Instance().RegisterBackground(shape);
}

void VisualTreeWatcher::ScanExistingViaHitTest() {
  Log(L"ScanExistingViaHitTest begin");
  auto consider = [&](InstanceHandle handle) {
    try {
      auto insp = FromHandle<winrt::Windows::Foundation::IInspectable>(handle);
      if (auto shape = insp.try_as<wuxs::Shape>()) {
        TryRegisterShape(shape);
        return;
      }
      if (auto fe = insp.try_as<wux::FrameworkElement>()) {
        // Shallow walk only — deep recursion previously risked Explorer instability.
        if (auto shape = fe.try_as<wuxs::Shape>()) {
          TryRegisterShape(shape);
        }
        const int n = wuxm::VisualTreeHelper::GetChildrenCount(fe);
        for (int i = 0; i < n; ++i) {
          auto child = wuxm::VisualTreeHelper::GetChild(fe, i);
          if (auto shape = child.try_as<wuxs::Shape>()) {
            TryRegisterShape(shape);
          }
          try {
            if (auto named = child.try_as<wux::FrameworkElement>();
                named && named.Name() == L"BackgroundFill") {
              if (auto shape = named.try_as<wuxs::Shape>()) TryRegisterShape(shape);
            }
          } catch (...) {
          }
        }
        auto parent = wuxm::VisualTreeHelper::GetParent(fe);
        for (int i = 0; i < 6 && parent; ++i) {
          try {
            if (auto named = parent.try_as<wux::FrameworkElement>();
                named && named.Name() == L"BackgroundFill") {
              if (auto shape = named.try_as<wuxs::Shape>()) TryRegisterShape(shape);
            }
          } catch (...) {
          }
          parent = wuxm::VisualTreeHelper::GetParent(parent);
        }
      }
    } catch (...) {
    }
  };

  if (HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    HitTestWindow(xamlDiagnostics_.get(), tray, consider);
  }
  HWND secondary = nullptr;
  while ((secondary = FindWindowExW(nullptr, secondary, L"Shell_SecondaryTrayWnd", nullptr))) {
    HitTestWindow(xamlDiagnostics_.get(), secondary, consider);
  }
  Log(L"ScanExistingViaHitTest end");
}

HRESULT STDMETHODCALLTYPE VisualTreeWatcher::OnVisualTreeChange(ParentChildRelation,
                                                                VisualElement element,
                                                                VisualMutationType mutationType) try {
  const std::wstring_view type{element.Type ? element.Type : L"",
                               element.Type ? SysStringLen(element.Type) : 0};
  const std::wstring_view name{element.Name ? element.Name : L"",
                               element.Name ? SysStringLen(element.Name) : 0};

  const auto freeElementStrings = [&] {
    if (element.Type) SysFreeString(element.Type);
    if (element.Name) SysFreeString(element.Name);
    if (element.SrcInfo.FileName) SysFreeString(element.SrcInfo.FileName);
    if (element.SrcInfo.Hash) SysFreeString(element.SrcInfo.Hash);
  };

  switch (mutationType) {
    case Add: {
      if (TypeIsRectangle(type) && NameIsBackgroundFill(name)) {
        try {
          TryRegisterShape(FromHandle<wuxs::Shape>(element.Handle));
        } catch (...) {
          Log(L"BackgroundFill FromHandle failed");
        }
      }
      break;
    }
    case Remove:
      TaskbarAppearanceService::Instance().UnregisterHandle(element.Handle);
      break;
  }

  freeElementStrings();
  return S_OK;
} catch (...) {
  return winrt::to_hresult();
}

HRESULT STDMETHODCALLTYPE VisualTreeWatcher::OnElementStateChanged(InstanceHandle, VisualElementState,
                                                                   LPCWSTR) noexcept {
  return S_OK;
}

}  // namespace phostide
