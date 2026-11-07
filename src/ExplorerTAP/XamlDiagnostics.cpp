#include "TaskbarAppearanceService.h"

#include <atomic>
#include <string>
#include <vector>

#include <UIAutomation.h>
#include <windows.h>

#include <winrt/base.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>

namespace wux = winrt::Windows::UI::Xaml;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxs = winrt::Windows::UI::Xaml::Shapes;

namespace phostide {
namespace {

std::atomic<bool> g_diagStarted{false};

void Walk(wux::DependencyObject const& node, int depth) {
  if (!node || depth > 28) return;
  if (auto shape = node.try_as<wuxs::Shape>()) {
    try {
      const auto name = shape.Name();
      if (name == L"BackgroundFill") {
        TaskbarAppearanceService::Instance().RegisterBackground(shape);
      }
    } catch (...) {
    }
  }
  const int count = wuxm::VisualTreeHelper::GetChildrenCount(node);
  for (int i = 0; i < count; ++i) {
    Walk(wuxm::VisualTreeHelper::GetChild(node, i), depth + 1);
  }
}

// UI Automation fallback: locate taskbar automation tree nodes named BackgroundFill
// and resolve their RuntimeId — used as a discovery signal that the TAP is alive.
void ScanViaUIAutomation() {
  IUIAutomation* automation = nullptr;
  if (FAILED(CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER,
                              IID_IUIAutomation, reinterpret_cast<void**>(&automation))) ||
      !automation) {
    return;
  }

  IUIAutomationElement* root = nullptr;
  if (FAILED(automation->GetRootElement(&root)) || !root) {
    automation->Release();
    return;
  }

  IUIAutomationCondition* cond = nullptr;
  VARIANT name{};
  name.vt = VT_BSTR;
  name.bstrVal = SysAllocString(L"BackgroundFill");
  automation->CreatePropertyCondition(UIA_NamePropertyId, name, &cond);
  SysFreeString(name.bstrVal);

  if (cond) {
    IUIAutomationElementArray* found = nullptr;
    if (SUCCEEDED(root->FindAll(TreeScope_Descendants, cond, &found)) && found) {
      int length = 0;
      found->get_Length(&length);
      // Presence confirms the XAML name exists in the desktop tree.
      // Full brush replacement still requires the DependencyObject from the XAML TAP site.
      found->Release();
    }
    cond->Release();
  }
  root->Release();
  automation->Release();
}

using InitializeXamlDiagnosticsExFn = HRESULT(WINAPI*)(
    LPCWSTR endPointName, DWORD pid, LPCWSTR wszDllXamlDiagnostics, LPCWSTR wszTAPDllName,
    CLSID tapClsid, LPCWSTR wszDllAppModule);

bool TryInitializeXamlDiagnostics() {
  HMODULE xaml = LoadLibraryW(L"Windows.UI.Xaml.dll");
  if (!xaml) return false;
  auto fn = reinterpret_cast<InitializeXamlDiagnosticsExFn>(
      GetProcAddress(xaml, "InitializeXamlDiagnosticsEx"));
  if (!fn) return false;

  // Connection name must be unique per attempt; TAP CLSID is reserved for a future
  // VisualTreeService site. Even without a registered site CLSID, loading our DLL
  // into Explorer via the host hook still runs the pipe + render loop.
  wchar_t endpoint[64];
  swprintf_s(endpoint, L"PhostideDiag_%lu", GetCurrentProcessId());
  const HRESULT hr = fn(endpoint, GetCurrentProcessId(), nullptr, nullptr, CLSID_NULL, nullptr);
  return SUCCEEDED(hr) || hr == 0x800700B7;  // already initialized is OK
}

DWORD WINAPI RescanThread(LPVOID) {
  for (int i = 0; i < 60; ++i) {
    ScanViaUIAutomation();
    Sleep(500);
  }
  return 0;
}

}  // namespace

void ScanFromRoot(wux::DependencyObject const& root) {
  Walk(root, 0);
}

bool StartXamlDiagnosticsBootstrap() {
  if (g_diagStarted.exchange(true)) return true;
  TryInitializeXamlDiagnostics();
  TaskbarAppearanceService::Instance().StartRenderLoop();
  CreateThread(nullptr, 0, RescanThread, nullptr, 0, nullptr);
  return true;
}

}  // namespace phostide
