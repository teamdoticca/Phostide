#include "../Common/PhostideCommon.h"

#include <windows.h>

namespace phostide {
namespace {

// Undocumented accent policy — used only as a fallback when XAML BackgroundFill
// has not been captured yet (classic / mixed taskbar, or TAP site not ready).
enum ACCENT_STATE {
  ACCENT_DISABLED = 0,
  ACCENT_ENABLE_GRADIENT = 1,
  ACCENT_ENABLE_TRANSPARENTGRADIENT = 2,
  ACCENT_ENABLE_BLURBEHIND = 3,
  ACCENT_ENABLE_ACRYLICBLURBEHIND = 4,
};

struct ACCENT_POLICY {
  ACCENT_STATE AccentState;
  DWORD AccentFlags;
  DWORD GradientColor;  // ABGR
  DWORD AnimationId;
};

struct WINDOWCOMPOSITIONATTRIBDATA {
  DWORD Attrib;
  PVOID pvData;
  SIZE_T cbData;
};

constexpr DWORD WCA_ACCENT_POLICY = 19;

using SetWindowCompositionAttributeFn = BOOL(WINAPI*)(HWND, WINDOWCOMPOSITIONATTRIBDATA*);

DWORD PackAbgr(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
  return (static_cast<DWORD>(a) << 24) | (static_cast<DWORD>(b) << 16) |
         (static_cast<DWORD>(g) << 8) | static_cast<DWORD>(r);
}

void ApplyAccent(HWND hwnd, DWORD abgr) {
  static auto fn = reinterpret_cast<SetWindowCompositionAttributeFn>(
      GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetWindowCompositionAttribute"));
  if (!fn || !hwnd) return;
  ACCENT_POLICY policy{ACCENT_ENABLE_TRANSPARENTGRADIENT, 2, abgr, 0};
  WINDOWCOMPOSITIONATTRIBDATA data{WCA_ACCENT_POLICY, &policy, sizeof(policy)};
  fn(hwnd, &data);
}

}  // namespace

void ApplyFallbackAccentToTaskbars(const ThemeCommand& cmd) {
  if (!cmd.enabled) return;
  const float intensity = cmd.intensity / 100.f;
  uint8_t r = 30, g = 40, b = 60, a = static_cast<uint8_t>(80 + 100 * intensity);
  switch (static_cast<ThemeId>(cmd.theme)) {
    case ThemeId::Rainbow: r = 80; g = 40; b = 90; break;
    case ThemeId::SeaWaves: r = 20; g = 70; b = 110; break;
    case ThemeId::Weather: r = g = b = 55; break;
    case ThemeId::Space: r = 25; g = 15; b = 50; break;
    default: break;
  }
  const DWORD color = PackAbgr(a, r, g, b);
  if (HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    ApplyAccent(tray, color);
  }
  HWND secondary = nullptr;
  while ((secondary = FindWindowExW(nullptr, secondary, L"Shell_SecondaryTrayWnd", nullptr))) {
    ApplyAccent(secondary, color);
  }
}

}  // namespace phostide
