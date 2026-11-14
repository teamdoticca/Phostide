#pragma once

#include <string_view>
#include <unordered_set>

#include <windows.h>
#include <xamlOM.h>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#pragma pop_macro("GetCurrentTime")

#include <winrt/base.h>

namespace phostide {

struct VisualTreeWatcher : winrt::implements<VisualTreeWatcher, IVisualTreeServiceCallback2> {
  VisualTreeWatcher(winrt::com_ptr<IUnknown> site, HANDLE readyEvent);

  VisualTreeWatcher(const VisualTreeWatcher&) = delete;
  VisualTreeWatcher& operator=(const VisualTreeWatcher&) = delete;

  HRESULT STDMETHODCALLTYPE OnVisualTreeChange(ParentChildRelation relation, VisualElement element,
                                               VisualMutationType mutationType) override;
  HRESULT STDMETHODCALLTYPE OnElementStateChanged(InstanceHandle element, VisualElementState elementState,
                                                  LPCWSTR context) noexcept override;

  void ScanExistingViaHitTest();

 private:
  template <typename T>
  T FromHandle(InstanceHandle handle) {
    winrt::Windows::Foundation::IInspectable obj{nullptr};
    winrt::check_hresult(xamlDiagnostics_->GetIInspectableFromHandle(
        handle, reinterpret_cast<::IInspectable**>(winrt::put_abi(obj))));
    return obj.as<T>();
  }

  void TryRegisterShape(winrt::Windows::UI::Xaml::Shapes::Shape const& shape,
                        HWND trayHwnd = nullptr,
                        InstanceHandle diagnosticsHandle = 0);
  void Log(const wchar_t* msg);

  winrt::com_ptr<IXamlDiagnostics> xamlDiagnostics_;
  std::unordered_set<InstanceHandle> pendingXamlSources_;
  HANDLE readyEvent_ = nullptr;
};

}  // namespace phostide
