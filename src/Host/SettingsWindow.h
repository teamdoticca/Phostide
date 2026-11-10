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
  // Commit appearance fields already written into *settings.
  void (*onApplyAppearance)() = nullptr;
  void (*onRequestUpdateCheck)() = nullptr;
  void (*onOpenUpdate)() = nullptr;
};

void SettingsWindowBind(const SettingsWindowHost& host);
void SettingsWindowShow(HWND owner, HINSTANCE instance, SettingsFocus focus);
void SettingsWindowRefreshUpdateUi();
bool SettingsWindowIsOpen();
HWND SettingsWindowHwnd();

}  // namespace phostide
