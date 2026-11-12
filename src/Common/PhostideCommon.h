#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <guiddef.h>

namespace phostide {

inline constexpr std::wstring_view kAppName = L"Phostide";
// Keep in sync with scripts/package-release.ps1 -Version default and GitHub release tags (vX.Y.Z).
inline constexpr std::wstring_view kAppVersion = L"0.1.13";
// Stable across installs/updates so Windows taskbar / Start treat us as the same app.
inline constexpr std::wstring_view kAppUserModelId = L"Doticca.Phostide";
inline constexpr std::wstring_view kMutexName = L"Local\\Phostide.SingleInstance";
inline constexpr std::wstring_view kTapReadyEvent = L"Local\\Phostide.TAP.Ready";
inline constexpr std::wstring_view kTapBoundEvent = L"Local\\Phostide.TAP.Bound";
// Written by ExplorerTAP on bind; host compares to kAppVersion to detect stale in-Explorer DLLs.
inline constexpr std::wstring_view kTapVersionStamp = L"tap.version";
inline constexpr std::wstring_view kSettingsFileName = L"settings.json";
// v4: ThemeCommand monitor slots — per-display theme/intensity/enabled (renderMode is global).
inline constexpr int kTapApiVersion = 4;
inline constexpr uint32_t kMaxMonitorSlots = 8;

enum class RenderMode : uint32_t {
  Auto = 0,  // D3D11 hardware if available, else CPU
  Gpu = 1,   // prefer GPU; fall back to CPU on failure
  Cpu = 2,   // WriteableBitmap PixelThemes
};

inline constexpr const wchar_t* RenderModeName(RenderMode mode) {
  switch (mode) {
    case RenderMode::Gpu: return L"GPU";
    case RenderMode::Cpu: return L"CPU";
    case RenderMode::Auto:
    default:              return L"Auto";
  }
}

inline RenderMode RenderModeFromIndex(int index) {
  if (index < 0 || index > 2) return RenderMode::Auto;
  return static_cast<RenderMode>(index);
}

// {A7C3E91F-4B2D-4E8A-9F15-6D3C8B0A1E72}
inline constexpr GUID kTapSiteClsid = {
    0xa7c3e91f, 0x4b2d, 0x4e8a, {0x9f, 0x15, 0x6d, 0x3c, 0x8b, 0x0a, 0x1e, 0x72}};

// Stable tray identity (NIF_GUID). Do not change — Windows notification prefs key off this.
// {B8D4F0A2-5C3E-4F9B-8A2D-7E6F1C9B3A45}
inline constexpr GUID kTrayIconGuid = {
    0xb8d4f0a2, 0x5c3e, 0x4f9b, {0x8a, 0x2d, 0x7e, 0x6f, 0x1c, 0x9b, 0x3a, 0x45}};

enum class ThemeId : uint32_t {
  Rainbow = 0,
  SeaWaves = 1,
  Weather = 2,
  Space = 3,
  Aurora = 4,
  Count
};

inline constexpr const wchar_t* ThemeName(ThemeId id) {
  switch (id) {
    case ThemeId::Rainbow:  return L"Rainbow";
    case ThemeId::SeaWaves: return L"Sea Waves";
    case ThemeId::Weather:  return L"Weather";
    case ThemeId::Space:    return L"Space";
    case ThemeId::Aurora:   return L"Aurora";
    default:                return L"Unknown";
  }
}

inline ThemeId ThemeFromIndex(int index) {
  if (index < 0 || index >= static_cast<int>(ThemeId::Count)) {
    return ThemeId::Rainbow;
  }
  return static_cast<ThemeId>(index);
}

struct MonitorAppearance {
  ThemeId theme = ThemeId::Rainbow;
  int intensity = 45;  // 0-100
};

struct MonitorAppearanceEntry {
  std::wstring deviceName;
  MonitorAppearance appearance;
};

struct Settings {
  bool enabled = true;
  // Fallbacks for brand-new monitors / migration from single-theme settings.
  ThemeId theme = ThemeId::Rainbow;
  int intensity = 45;  // 0-100
  bool startWithWindows = false;
  bool pauseWhenLocked = true;
  // App-global (System tab) — not per-monitor.
  RenderMode renderMode = RenderMode::Auto;
  // Device names that are off; empty = all monitors active.
  std::vector<std::wstring> disabledMonitors;
  // Per-monitor theme/intensity; missing key → theme/intensity fallbacks.
  std::vector<MonitorAppearanceEntry> monitorSettings;
  // Update check (GitHub Releases)
  int64_t lastUpdateCheckUtc = 0;  // unix seconds; 0 = never
  std::wstring pendingUpdateTag;   // e.g. "v0.1.3" when newer than kAppVersion
  std::wstring pendingSetupUrl;    // direct Setup.exe download URL
};

#pragma pack(push, 1)
struct MonitorPaintSlot {
  uint64_t deviceHash = 0;
  uint32_t theme = 0;
  uint32_t intensity = 45;
  uint32_t enabled = 1;
};

struct ThemeCommand {
  uint32_t apiVersion = kTapApiVersion;
  // Fallbacks / legacy TAP (< v4) + accent path.
  uint32_t theme = 0;
  uint32_t intensity = 45;
  uint32_t enabled = 1;
  // 1 = keep last frame / bound brushes, stop the UI timer (e.g. session lock).
  uint32_t paused = 0;
  uint32_t renderMode = static_cast<uint32_t>(RenderMode::Auto);
  uint32_t slotCount = 0;
  MonitorPaintSlot slots[kMaxMonitorSlots]{};
};
#pragma pack(pop)

}  // namespace phostide
