#pragma once

#include "PhostideCommon.h"

#include <cstdint>

namespace phostide {

struct AccentColor {
  uint8_t a = 0;
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;
};

// Samples a theme color at timeSeconds (host-side animation; accent is solid only).
AccentColor SampleThemeAccent(ThemeId theme, float intensity0to1, float timeSeconds);

// Undocumented accent policy on Shell_TrayWnd — safe from any process.
void ApplyTaskbarAccentColor(const AccentColor& color);
void ApplyTaskbarAccent(const ThemeCommand& cmd, float timeSeconds = 0.f);
void ClearTaskbarAccent();

}  // namespace phostide
