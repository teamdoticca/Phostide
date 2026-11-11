#pragma once

#include "../Common/PhostideCommon.h"
#include "GpuThemeRenderer.h"
#include "TaskbarGeometry.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include <windows.h>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#include <winrt/Windows.UI.Xaml.Media.Imaging.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#include <winrt/Windows.System.h>
#pragma pop_macro("GetCurrentTime")

namespace phostide {

struct TaskbarVisual {
  winrt::Windows::UI::Xaml::Shapes::Shape background{nullptr};
  winrt::Windows::UI::Xaml::Media::Brush originalFill{nullptr};
  winrt::Windows::UI::Xaml::Media::Imaging::WriteableBitmap bitmap{nullptr};
  winrt::Windows::UI::Xaml::Media::ImageBrush imageBrush{nullptr};
  TaskbarGeom geom{};
  int pixelWidth = 0;
  int pixelHeight = 0;
  bool bound = false;
  bool gpuActive = false;
};

class TaskbarAppearanceService {
 public:
  static TaskbarAppearanceService& Instance();

  void RegisterBackground(winrt::Windows::UI::Xaml::Shapes::Shape const& shape);
  void UnregisterHandle(uint64_t unusedHandle);
  void Apply(const ThemeCommand& cmd);
  void RestoreAll();
  void StartRenderLoop();
  void StopRenderLoop();
  void WatchHostMutex();
  bool HasBoundFill() const { return boundCount_.load() > 0; }

 private:
  TaskbarAppearanceService() = default;

  void EnsureUiTimer();
  void StopUiTimer();
  void TickUi();
  bool BindVisual(TaskbarVisual& tv);
  void UnbindVisual(TaskbarVisual& tv);
  void RefreshGeomAndBitmap(TaskbarVisual& tv);
  void PaintVisual(TaskbarVisual& tv, float timeSeconds);
  void PaintVisualCpu(TaskbarVisual& tv, float timeSeconds);
  bool WantGpu() const;
  bool TryEnableGpuFor(TaskbarVisual& tv);
  void SignalBound();

  std::mutex mutex_;
  std::vector<TaskbarVisual> taskbars_;
  ThemeCommand command_{};
  std::atomic<bool> running_{false};
  std::atomic<int> boundCount_{0};
  unsigned paintFrame_{0};
  std::thread watchThread_;
  std::chrono::steady_clock::time_point start_{std::chrono::steady_clock::now()};
  winrt::Windows::System::DispatcherQueue dispatcher_{nullptr};
  winrt::Windows::System::DispatcherQueueTimer timer_{nullptr};
  GpuThemeRenderer gpu_;
  bool gpuFailedThisSession_{false};
};

}  // namespace phostide
