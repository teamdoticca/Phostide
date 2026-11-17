#pragma once

#include "UpdateCheck.h"
#include "../Common/PhostideCommon.h"

#include <windows.h>

namespace phostide {

enum class SettingsFocus {
  Normal,
  Updates,
};

struct SettingsWindowHost {
  Settings* settings = nullptr;
  bool* updateCheckRunning = nullptr;
  // Optional last check message (errors); empty uses default status text.
  std::wstring* updateStatusText = nullptr;
  // Live host pause flags (applied settings + session / fullscreen detectors).
  bool* sessionLocked = nullptr;
  bool* fullscreenPaused = nullptr;
  const uint64_t* fullscreenHashes = nullptr;
  uint32_t* fullscreenHashCount = nullptr;
  // Commit appearance fields already written into *settings.
  void (*onApplyAppearance)() = nullptr;
  void (*onRequestUpdateCheck)() = nullptr;
  void (*onOpenUpdate)() = nullptr;
};

void SettingsWindowBind(const SettingsWindowHost& host);
void SettingsWindowShow(HWND owner, HINSTANCE instance, SettingsFocus focus);
void SettingsWindowRefreshUpdateUi();
void SettingsWindowRefreshPauseUi();
bool SettingsWindowIsOpen();
HWND SettingsWindowHwnd();

}  // namespace phostide
