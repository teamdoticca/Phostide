#pragma once

#include "ITaskbarEffect.h"
#include "Themes.h"

#include <chrono>
#include <memory>
#include <mutex>

#include <d2d1_1.h>
#include <d3d11.h>
#include <dcomp.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

namespace phostide {

// Renders the active theme into a CompositionDrawingSurface each frame.
class EffectEngine {
 public:
  EffectEngine();
  ~EffectEngine();

  bool Initialize(float width, float height);
  void Resize(float width, float height);
  void SetTheme(ThemeId id);
  void SetIntensity(int intensity0to100);
  void SetEnabled(bool enabled);
  void Tick();  // draw one frame into the surface

  ICompositionSurface* Surface() const { return surface_.Get(); }
  bool IsReady() const { return ready_; }

 private:
  bool CreateGraphics(float width, float height);
  void DestroyGraphics();

  Microsoft::WRL::ComPtr<ID3D11Device> d3d_;
  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_;
  Microsoft::WRL::ComPtr<ID2D1Device> d2dDevice_;
  Microsoft::WRL::ComPtr<ID2D1DeviceContext> d2dDc_;
  Microsoft::WRL::ComPtr<ICompositionGraphicsDevice> compGraphics_;
  Microsoft::WRL::ComPtr<ICompositionDrawingSurface> surface_;

  std::unique_ptr<ITaskbarEffect> effect_;
  ThemeId theme_ = ThemeId::Rainbow;
  float intensity_ = 0.45f;
  bool enabled_ = true;
  bool ready_ = false;
  float width_ = 0.f;
  float height_ = 0.f;
  std::chrono::steady_clock::time_point start_;
  std::mutex mutex_;
};

}  // namespace phostide
