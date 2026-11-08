#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include <guiddef.h>

namespace phostide {

inline constexpr std::wstring_view kAppName = L"Phostide";
inline constexpr std::wstring_view kMutexName = L"Local\\Phostide.SingleInstance";
inline constexpr std::wstring_view kTapReadyEvent = L"Local\\Phostide.TAP.Ready";
inline constexpr std::wstring_view kTapBoundEvent = L"Local\\Phostide.TAP.Bound";
inline constexpr std::wstring_view kSettingsFileName = L"settings.json";
inline constexpr int kTapApiVersion = 1;

// {A7C3E91F-4B2D-4E8A-9F15-6D3C8B0A1E72}
inline constexpr GUID kTapSiteClsid = {
    0xa7c3e91f, 0x4b2d, 0x4e8a, {0x9f, 0x15, 0x6d, 0x3c, 0x8b, 0x0a, 0x1e, 0x72}};

enum class ThemeId : uint32_t {
  Rainbow = 0,
  SeaWaves = 1,
  Weather = 2,
  Space = 3,
  Count
};

inline constexpr const wchar_t* ThemeName(ThemeId id) {
  switch (id) {
    case ThemeId::Rainbow:  return L"Rainbow";
    case ThemeId::SeaWaves: return L"Sea Waves";
    case ThemeId::Weather:  return L"Weather";
    case ThemeId::Space:    return L"Space";
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
