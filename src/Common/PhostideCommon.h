#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <guiddef.h>

namespace phostide {

inline constexpr std::wstring_view kAppName = L"Phostide";
// Keep in sync with scripts/package-release.ps1 -Version default and GitHub release tags (vX.Y.Z).
inline constexpr std::wstring_view kAppVersion = L"0.1.6";
// Stable across installs/updates so Windows taskbar / Start treat us as the same app.
inline constexpr std::wstring_view kAppUserModelId = L"Doticca.Phostide";
inline constexpr std::wstring_view kMutexName = L"Local\\Phostide.SingleInstance";
inline constexpr std::wstring_view kTapReadyEvent = L"Local\\Phostide.TAP.Ready";
inline constexpr std::wstring_view kTapBoundEvent = L"Local\\Phostide.TAP.Bound";
inline constexpr std::wstring_view kSettingsFileName = L"settings.json";
inline constexpr int kTapApiVersion = 1;

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

struct Settings {
  bool enabled = true;
  ThemeId theme = ThemeId::Rainbow;
  int intensity = 45;          // 0-100
  bool startWithWindows = false;
  bool pauseWhenLocked = true;
  // Update check (GitHub Releases)
  int64_t lastUpdateCheckUtc = 0;  // unix seconds; 0 = never
  std::wstring pendingUpdateTag;   // e.g. "v0.1.3" when newer than kAppVersion
  std::wstring pendingSetupUrl;    // direct Setup.exe download URL
};

#pragma pack(push, 1)
struct ThemeCommand {
  uint32_t apiVersion = kTapApiVersion;
  uint32_t theme = 0;
  uint32_t intensity = 45;
  uint32_t enabled = 1;
};
#pragma pack(pop)

}  // namespace phostide
