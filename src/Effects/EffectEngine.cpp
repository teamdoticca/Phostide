#include "EffectEngine.h"

#include <windows.ui.composition.interop.h>
#include <winrt/Windows.UI.Composition.h>

using Microsoft::WRL::ComPtr;
namespace wuc = winrt::Windows::UI::Composition;

namespace phostide {

EffectEngine::EffectEngine() : start_(std::chrono::steady_clock::now()) {
  effect_ = CreateEffect(ThemeId::Rainbow);
}

EffectEngine::~EffectEngine() {
  DestroyGraphics();
}

bool EffectEngine::Initialize(float width, float height) {
  std::lock_guard lock(mutex_);
  return CreateGraphics(width, height);
}

void EffectEngine::Resize(float width, float height) {
  std::lock_guard lock(mutex_);
  if (std::abs(width - width_) < 0.5f && std::abs(height - height_) < 0.5f) {
    return;
  }
  DestroyGraphics();
  CreateGraphics(width, height);
}

void EffectEngine::SetTheme(ThemeId id) {
  std::lock_guard lock(mutex_);
  if (theme_ == id && effect_) return;
  theme_ = id;
  effect_ = CreateEffect(id);
}

void EffectEngine::SetIntensity(int intensity0to100) {
  std::lock_guard lock(mutex_);
  intensity_ = (std::max)(0, (std::min)(100, intensity0to100)) / 100.f;
}

void EffectEngine::SetEnabled(bool enabled) {
  std::lock_guard lock(mutex_);
  enabled_ = enabled;
}

bool EffectEngine::CreateGraphics(float width, float height) {
  width_ = (std::max)(1.f, width);
  height_ = (std::max)(1.f, height);
  ready_ = false;

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#if defined(_DEBUG)
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL level;
  const D3D_FEATURE_LEVEL levels[] = {
      D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};

  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels,
                                 ARRAYSIZE(levels), D3D11_SDK_VERSION, &d3d_, &level, nullptr);
  if (FAILED(hr)) {
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, levels, ARRAYSIZE(levels),
                           D3D11_SDK_VERSION, &d3d_, &level, nullptr);
  }
  if (FAILED(hr)) return false;

  hr = d3d_.As(&dxgi_);
  if (FAILED(hr)) return false;

  ComPtr<ID2D1Factory1> d2dFactory;
  D2D1_FACTORY_OPTIONS opts{};
  hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, opts, d2dFactory.GetAddressOf());
  if (FAILED(hr)) return false;

  hr = d2dFactory->CreateDevice(dxgi_.Get(), &d2dDevice_);
  if (FAILED(hr)) return false;

  hr = d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDc_);
  if (FAILED(hr)) return false;

  // Composition surface is created by the TAP on the XAML compositor thread.
  // Host-side engine keeps D2D ready; TAP binds the surface via SetSurfaceInterop.
  ready_ = true;
  return true;
}

void EffectEngine::DestroyGraphics() {
  surface_.Reset();
  compGraphics_.Reset();
  d2dDc_.Reset();
  d2dDevice_.Reset();
  dxgi_.Reset();
  d3d_.Reset();
  ready_ = false;
}

void EffectEngine::Tick() {
  std::lock_guard lock(mutex_);
  if (!ready_ || !d2dDc_ || !effect_) return;

  // When a composition surface is bound, draw into it. Otherwise this is a no-op
  // until ExplorerTAP attaches the surface (see BindSurface).
  if (!surface_) return;

  ComPtr<ABI::Windows::UI::Composition::ICompositionDrawingSurfaceInterop> interop;
  if (FAILED(surface_.As(&interop))) return;

  POINT offset{};
  ComPtr<IDXGISurface> dxgiSurface;
  HRESULT hr = interop->BeginDraw(nullptr, IID_PPV_ARGS(&dxgiSurface), &offset);
  if (FAILED(hr)) {
    if (hr == E_SURFACE_CONTENTS_LOST) {
      // Caller should recreate surface.
    }
    return;
  }

  ComPtr<ID2D1Bitmap1> bitmap;
  const D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
      D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
      D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

  hr = d2dDc_->CreateBitmapFromDxgiSurface(dxgiSurface.Get(), &props, &bitmap);
  if (SUCCEEDED(hr)) {
    d2dDc_->SetTarget(bitmap.Get());
    d2dDc_->BeginDraw();
    if (enabled_) {
      const auto now = std::chrono::steady_clock::now();
      const float t =
          std::chrono::duration<float>(now - start_).count();
      EffectContext ctx{width_, height_, t, intensity_};
      d2dDc_->Clear(D2D1::ColorF(0, 0, 0, 0));
      effect_->Update(d2dDc_.Get(), ctx);
    } else {
      d2dDc_->Clear(D2D1::ColorF(0, 0, 0, 0));
    }
    d2dDc_->EndDraw();
    d2dDc_->SetTarget(nullptr);
  }
  interop->EndDraw();
}

}  // namespace phostide
