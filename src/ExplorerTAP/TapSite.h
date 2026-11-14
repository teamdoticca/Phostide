#pragma once

#include <ocidl.h>
#include <windows.h>

#include <winrt/base.h>

namespace phostide {

struct VisualTreeWatcher;

struct TAPSite : winrt::implements<TAPSite, IObjectWithSite> {
  static DWORD WINAPI Install(void* parameter);
  static HANDLE ReadyEvent();
  // Re-hit-test tray windows for BackgroundFill after a same-process tree rebuild.
  static void RequestRescan();

  HRESULT STDMETHODCALLTYPE SetSite(IUnknown* pUnkSite) override;
  HRESULT STDMETHODCALLTYPE GetSite(REFIID riid, void** ppvSite) noexcept override;

 private:
  winrt::com_ptr<IUnknown> site_;
  static winrt::weak_ref<VisualTreeWatcher> s_watcher_;
};

}  // namespace phostide
