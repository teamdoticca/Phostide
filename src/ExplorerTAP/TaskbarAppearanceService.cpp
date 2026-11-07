#include "TaskbarAppearanceService.h"

#include <algorithm>
#include <cmath>

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.h>

using Microsoft::WRL::ComPtr;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxs = winrt::Windows::UI::Xaml::Shapes;
namespace wuc = winrt::Windows::UI::Composition;
namespace wuxh = winrt::Windows::UI::Xaml::Hosting;

namespace phostide {

TaskbarAppearanceService& TaskbarAppearanceService::Instance() {
  static TaskbarAppearanceService instance;
  return instance;
}

void TaskbarAppearanceService::RegisterBackground(wuxs::Shape const& shape) {
  if (!shape) return;
  std::lock_guard lock(mutex_);
  for (auto& tv : taskbars_) {
    if (tv.background == shape) return;
  }
  TaskbarVisual tv;
  tv.background = shape;
  tv.originalFill = shape.Fill();
  const auto size = shape.RenderSize();
  tv.width = static_cast<float>((std::max)(1.0, static_cast<double>(size.Width)));
  tv.height = static_cast<float>((std::max)(1.0, static_cast<double>(size.Height)));
  taskbars_.push_back(std::move(tv));
}

void TaskbarAppearanceService::Apply(const ThemeCommand& cmd) {
  std::lock_guard lock(mutex_);
  command_ = cmd;
  effect_ = CreateEffect(static_cast<ThemeId>(cmd.theme));
}

void TaskbarAppearanceService::RestoreAll() {
  std::lock_guard lock(mutex_);
  for (auto& tv : taskbars_) {
    if (tv.background && tv.originalFill) {
      try {
        tv.background.Fill(tv.originalFill);
      } catch (...) {
      }
    }
    tv.brush = nullptr;
    tv.surface = nullptr;
  }
}

bool TaskbarAppearanceService::EnsureGraphics() {
  if (d2dDc_) return true;

  UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
  D3D_FEATURE_LEVEL level{};
  const D3D_FEATURE_LEVEL levels[] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0};
  HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, levels,
                                 ARRAYSIZE(levels), D3D11_SDK_VERSION, &d3d_, &level, nullptr);
  if (FAILED(hr)) {
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, levels, ARRAYSIZE(levels),
                           D3D11_SDK_VERSION, &d3d_, &level, nullptr);
  }
  if (FAILED(hr)) return false;
  if (FAILED(d3d_.As(&dxgi_))) return false;

  ComPtr<ID2D1Factory1> factory;
  D2D1_FACTORY_OPTIONS opts{};
  if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory1), &opts,
                               reinterpret_cast<void**>(factory.GetAddressOf())))) {
    return false;
  }
  if (FAILED(factory->CreateDevice(dxgi_.Get(), &d2dDevice_))) return false;
  if (FAILED(d2dDevice_->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &d2dDc_))) {
    return false;
  }
  return true;
}

bool TaskbarAppearanceService::EnsureSurface(TaskbarVisual& tv, wuc::Compositor const&) {
  // DrawingSurface interop lands next; for now SolidColorBrush on BackgroundFill
  // still mutates the real taskbar Fill pixels once the shape is registered.
  if (!tv.background) return false;
  try {
    wuxm::SolidColorBrush solid;
    solid.Color(winrt::Windows::UI::Color{255, 20, 40, 55});
    solid.Opacity(0.55 + 0.35 * (command_.intensity / 100.0));
    tv.background.Fill(solid);
  } catch (...) {
    return false;
  }
  return true;
}

void ApplyFallbackAccentToTaskbars(const ThemeCommand& cmd);

void TaskbarAppearanceService::RenderFrame() {
  std::lock_guard lock(mutex_);
  if (!command_.enabled) {
    return;
  }
  if (taskbars_.empty()) {
    ApplyFallbackAccentToTaskbars(command_);
    return;
  }
  if (!effect_) return;
  if (!EnsureGraphics() || !d2dDc_) return;

  const auto now = std::chrono::steady_clock::now();
  const float t = std::chrono::duration<float>(now - start_).count();
  const float intensity = command_.intensity / 100.f;

  for (auto& tv : taskbars_) {
    if (!tv.background) continue;
    try {
      const auto size = tv.background.RenderSize();
      tv.width = static_cast<float>((std::max)(1.0, static_cast<double>(size.Width)));
      tv.height = static_cast<float>((std::max)(1.0, static_cast<double>(size.Height)));

      // Procedural color for the live brush — maps theme → real BackgroundFill.
      float r = 0.1f, g = 0.1f, b = 0.15f, a = 0.45f + 0.4f * intensity;
      switch (static_cast<ThemeId>(command_.theme)) {
        case ThemeId::Rainbow: {
          const float h = std::fmod(t * 0.05f, 1.f);
          const float i = std::floor(h * 6.f);
          const float f = h * 6.f - i;
          const float p = 0.2f * intensity;
          const float q = 0.35f * intensity * (1.f - f);
          const float u = 0.35f * intensity * f;
          switch (static_cast<int>(i) % 6) {
            case 0: r = 0.35f * intensity; g = u; b = p; break;
            case 1: r = q; g = 0.35f * intensity; b = p; break;
            case 2: r = p; g = 0.35f * intensity; b = u; break;
            case 3: r = p; g = q; b = 0.35f * intensity; break;
            case 4: r = u; g = p; b = 0.35f * intensity; break;
            default: r = 0.35f * intensity; g = p; b = q; break;
          }
          break;
        }
        case ThemeId::SeaWaves: {
          const float w = 0.5f + 0.5f * std::sin(t * 0.7f);
          r = 0.05f; g = 0.15f + 0.2f * w * intensity; b = 0.25f + 0.25f * intensity;
          break;
        }
        case ThemeId::Weather: {
          const float n = 0.5f + 0.5f * std::sin(t * 1.1f);
          r = g = b = 0.12f + 0.1f * n * intensity;
          break;
        }
        case ThemeId::Space: {
          r = 0.08f + 0.1f * intensity * (0.5f + 0.5f * std::sin(t * 0.2f));
          g = 0.05f;
          b = 0.18f + 0.15f * intensity;
          break;
        }
        default:
          break;
      }

      wuxm::SolidColorBrush solid;
      solid.Color(winrt::Windows::UI::Color{
          static_cast<uint8_t>(a * 255),
          static_cast<uint8_t>(r * 255),
          static_cast<uint8_t>(g * 255),
          static_cast<uint8_t>(b * 255)});
      tv.background.Fill(solid);
    } catch (...) {
    }
  }
}

void TaskbarAppearanceService::StartRenderLoop() {
  if (running_.exchange(true)) return;
  effect_ = CreateEffect(ThemeId::Rainbow);
  renderThread_ = std::thread([this] {
    while (running_) {
      RenderFrame();
      Sleep(33);  // ~30 fps — enough for OLED care, light on Explorer
    }
  });
}

void TaskbarAppearanceService::StopRenderLoop() {
  running_ = false;
  if (renderThread_.joinable()) renderThread_.join();
  RestoreAll();
}

void TaskbarAppearanceService::WatchHostProcess(DWORD pid) {
  if (watchThread_.joinable()) return;
  watchThread_ = std::thread([this, pid] {
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (!h) return;
    WaitForSingleObject(h, INFINITE);
    CloseHandle(h);
    StopRenderLoop();
  });
}

}  // namespace phostide
