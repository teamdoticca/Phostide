#pragma once

#include "../Common/PhostideCommon.h"
#include "../Effects/Themes.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <windows.h>
#include <wrl/client.h>

#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Controls.h>
#include <winrt/Windows.UI.Xaml.Hosting.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Windows.UI.Composition.h>
#include <windows.ui.composition.interop.h>

#include <d2d1_1.h>
#include <d3d11.h>

namespace phostide {

struct TaskbarVisual {
  winrt::Windows::UI::Xaml::Shapes::Shape background{nullptr};
  winrt::Windows::UI::Xaml::Media::Brush originalFill{nullptr};
  Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionDrawingSurface> surface;
  Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionSurfaceBrush> brush;
  float width = 0.f;
  float height = 0.f;
};

// Runs inside explorer.exe. Owns BackgroundFill brushes and the render loop.
class TaskbarAppearanceService {
 public:
  static TaskbarAppearanceService& Instance();

  void RegisterBackground(winrt::Windows::UI::Xaml::Shapes::Shape const& shape);
  void Apply(const ThemeCommand& cmd);
  void RestoreAll();
  void StartRenderLoop();
  void StopRenderLoop();
  void WatchHostProcess(DWORD pid);

 private:
  TaskbarAppearanceService() = default;

  void RenderFrame();
  bool EnsureGraphics();
  bool EnsureSurface(TaskbarVisual& tv, winrt::Windows::UI::Composition::Compositor const& compositor);

  std::mutex mutex_;
  std::vector<TaskbarVisual> taskbars_;
  ThemeCommand command_{};
  std::unique_ptr<ITaskbarEffect> effect_;
  std::atomic<bool> running_{false};
  std::thread renderThread_;
  std::thread watchThread_;
  std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};

  Microsoft::WRL::ComPtr<ID3D11Device> d3d_;
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_;
  Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
  Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dDc_;
};

}  // namespace phostide
