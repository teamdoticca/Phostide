#pragma once

#include "../Common/PhostideCommon.h"
#include "TaskbarGeometry.h"

#include <cstdint>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.UI.Composition.h>
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Shapes.h>
#pragma pop_macro("GetCurrentTime")

namespace phostide {

// D3D11 + CompositionDrawingSurface painter for BackgroundFill overlays.
class GpuThemeRenderer {
 public:
  GpuThemeRenderer() = default;
  ~GpuThemeRenderer();

  GpuThemeRenderer(const GpuThemeRenderer&) = delete;
  GpuThemeRenderer& operator=(const GpuThemeRenderer&) = delete;

  // Try D3D11 hardware init once. Returns false → caller must use CPU path.
  bool EnsureDevice();
  bool IsReady() const { return ready_; }
  void ResetDevice();

  // Attach / resize a SpriteVisual child that draws the GPU surface.
  bool Attach(winrt::Windows::UI::Xaml::Shapes::Shape const& shape, int pixelW, int pixelH);
  void Detach(winrt::Windows::UI::Xaml::Shapes::Shape const& shape);
  void Resize(winrt::Windows::UI::Xaml::Shapes::Shape const& shape, int pixelW, int pixelH);

  // Draw one frame into the surface for this shape. Returns false on device loss.
  bool Paint(winrt::Windows::UI::Xaml::Shapes::Shape const& shape, ThemeId theme,
             float intensity, float timeSeconds, TaskbarEdge edge, int pixelW, int pixelH);

  void DetachAll();

 private:
  struct Target;

  bool CreatePipeline();
  bool EnsureOffscreen(Target& target);
  bool EnsureTarget(winrt::Windows::UI::Xaml::Shapes::Shape const& shape, int pixelW,
                    int pixelH, Target** out);
  bool DrawTarget(Target& target, ThemeId theme, float intensity, float timeSeconds,
                  TaskbarEdge edge);

  bool ready_ = false;
  bool initAttempted_ = false;

  struct Impl;
  Impl* impl_ = nullptr;
};

}  // namespace phostide
