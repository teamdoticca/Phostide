#include "TaskbarAccent.h"

#include <algorithm>
#include <cmath>

#include <windows.h>

namespace phostide {
namespace {

enum ACCENT_STATE {
  ACCENT_DISABLED = 0,
  ACCENT_ENABLE_GRADIENT = 1,
  ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
};

struct ACCENT_POLICY {
  ACCENT_STATE AccentState;
  DWORD AccentFlags;
  DWORD GradientColor;
  DWORD AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
  DWORD Attrib;
  PVOID pvData;
  SIZE_T cbData;
};

constexpr DWORD WCA_ACCENT_POLICY = 19;

using SetWindowCompositionAttributeFn = BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

SetWindowCompositionAttributeFn GetSetAttr() {
  static auto fn = reinterpret_cast<SetWindowCompositionAttributeFn>(
      GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
  return fn;
}

DWORD PackAbgr(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return (static_cast<DWORD>(a) << 24) | (static_cast<DWORD>(b) << 16) |
         (static_cast<DWORD>(g) << 8) | static_cast<DWORD>(r);
}

void ApplyAccent(HWND hwnd, const ACCENT_POLICY& policy) {
  auto fn = GetSetAttr();
  if (!fn || !hwnd) return;
  WINDOWCOMPOSITIONATTRIBDATA data{WCA_ACCENT_POLICY, const_cast<ACCENT_POLICY*>(&policy),
                                   sizeof(policy)};
  fn(hwnd, &data);
}

void ForEachTaskbar(const ACCENT_POLICY& policy) {
  if (HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    ApplyAccent(tray, policy);
  }
  HWND secondary = nullptr;
  while ((secondary = FindWindowExW(nullptr, secondary, L"Shell_SecondaryTrayWnd", nullptr))) {
    ApplyAccent(secondary, policy);
  }
}

void HsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
  h = std::fmod(h, 1.f);
  if (h < 0.f) h += 1.f;
  const float i = std::floor(h * 6.f);
  const float f = h * 6.f - i;
  const float p = v * (1.f - s);
  const float q = v * (1.f - f * s);
  const float t = v * (1.f - (1.f - f) * s);
  switch (static_cast<int>(i) % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

uint8_t ToByte(float x) {
  return static_cast<uint8_t>(std::clamp(x, 0.f, 1.f) * 255.f + 0.5f);
}

}  // namespace

AccentColor SampleThemeAccent(ThemeId theme, float intensity0to1, float timeSeconds) {
  const float intensity = std::clamp(intensity0to1, 0.f, 1.f);
  const float t = timeSeconds;
  float r = 0.12f, g = 0.16f, b = 0.24f;
  float alpha = 0.35f + 0.45f * intensity;

  switch (theme) {
    case ThemeId::Rainbow: {
      // Full hue sweep — most visible motion with a solid accent fill.
      HsvToRgb(t * 0.12f, 0.85f, 0.28f + 0.35f * intensity, r, g, b);
      alpha = 0.40f + 0.50f * intensity;
      break;
    }
    case ThemeId::SeaWaves: {
      const float wave = 0.5f + 0.5f * std::sin(t * 1.1f);
      const float wave2 = 0.5f + 0.5f * std::sin(t * 0.55f + 1.7f);
      r = 0.04f + 0.06f * wave2;
      g = 0.22f + 0.28f * wave * (0.5f + 0.5f * intensity);
      b = 0.40f + 0.35f * wave2;
      alpha = 0.38f + 0.35f * intensity + 0.12f * wave;
      break;
    }
    case ThemeId::Weather: {
      const float flicker = 0.5f + 0.5f * std::sin(t * 1.4f);
      const float drift = 0.5f + 0.5f * std::sin(t * 0.35f);
      const float base = 0.18f + 0.22f * intensity;
      r = base + 0.08f * flicker;
      g = base + 0.06f * flicker;
      b = base + 0.10f + 0.08f * drift;
      alpha = 0.42f + 0.30f * intensity + 0.10f * flicker;
      break;
    }
    case ThemeId::Space: {
      const float pulse = 0.5f + 0.5f * std::sin(t * 2.2f);
      HsvToRgb(0.72f + 0.08f * std::sin(t * 0.25f), 0.65f,
               0.18f + 0.28f * intensity + 0.10f * pulse, r, g, b);
      alpha = 0.45f + 0.35f * intensity + 0.12f * pulse;
      break;
    }
    case ThemeId::Aurora: {
      // Cool green / teal accent — mirrors curtain palette without violet wash.
      const float breath = 0.5f + 0.5f * std::sin(t * 0.55f);
      const float teal = 0.5f + 0.5f * std::sin(t * 0.31f + 1.4f);
      r = 0.04f + 0.05f * teal;
      g = 0.28f + 0.32f * breath * (0.55f + 0.45f * intensity);
      b = 0.30f + 0.28f * teal + 0.10f * breath;
      alpha = 0.40f + 0.35f * intensity + 0.10f * breath;
      break;
    }
    default:
      break;
  }

  return AccentColor{ToByte(alpha), ToByte(r), ToByte(g), ToByte(b)};
}

void ApplyTaskbarAccentColor(const AccentColor& color) {
  ACCENT_POLICY policy{ACCENT_ENABLE_TRANSPARENTGRADIENT, 2,
                       PackAbgr(color.a, color.r, color.g, color.b), 0};
  ForEachTaskbar(policy);
}

void ApplyTaskbarAccent(const ThemeCommand& cmd, float timeSeconds) {
  if (!cmd.enabled) {
    ClearTaskbarAccent();
    return;
  }
  const auto color =
      SampleThemeAccent(static_cast<ThemeId>(cmd.theme), cmd.intensity / 100.f, timeSeconds);
  ApplyTaskbarAccentColor(color);
}

void ClearTaskbarAccent() {
  ACCENT_POLICY policy{ACCENT_DISABLED, 0, 0, 0};
  ForEachTaskbar(policy);
  if (HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    PostMessageW(tray, WM_DWMCOMPOSITIONCHANGED, 1, 0);
  }
}

}  // namespace phostide
