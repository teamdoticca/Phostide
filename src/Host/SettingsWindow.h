#pragma once

#include "UpdateCheck.h"
#include "../Common/PhostideCommon.h"

#include <windows.h>

namespace phostide {

enum class SettingsFocus {
  Normal,
  Updates,
  Welcome,  // first run: which displays are OLED, then Done / Customize
};

struct SettingsWindowHost {
  Settings* settings = nullptr;
  bool* updateCheckRunning = nullptr;
  // True while "Update now" downloads / verifies / starts Setup.
  bool* updateInstallRunning = nullptr;
  // Optional last check message (errors); empty uses default status text.
  std::wstring* updateStatusText = nullptr;
  // Live host pause flags (applied settings + session / fullscreen detectors).
  bool* sessionLocked = nullptr;
  bool* fullscreenPaused = nullptr;
  const uint64_t* fullscreenHashes = nullptr;
  uint32_t* fullscreenHashCount = nullptr;
  // Tray "Pause for 1 hour / until restart": e.g. "Paused until 15:42"; empty otherwise.
  std::wstring* pauseText = nullptr;
  // Commit appearance fields already written into *settings.
  void (*onApplyAppearance)() = nullptr;
  void (*onRequestUpdateCheck)() = nullptr;
  void (*onOpenUpdate)() = nullptr;
  // System tab "Copy diagnostics": builds the report and puts it on the clipboard.
  bool (*onCopyDiagnostics)(HWND owner) = nullptr;
};

void SettingsWindowBind(const SettingsWindowHost& host);
void SettingsWindowShow(HWND owner, HINSTANCE instance, SettingsFocus focus);
void SettingsWindowRefreshUpdateUi();
void SettingsWindowRefreshPauseUi();
// Theme/intensity changed outside the window (tray Theme menu): reload that draft.
void SettingsWindowReloadAppearance();
bool SettingsWindowIsOpen();
HWND SettingsWindowHwnd();

}  // namespace phostide
