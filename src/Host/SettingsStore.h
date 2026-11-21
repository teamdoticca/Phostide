#pragma once

#include "../Common/PhostideCommon.h"

namespace phostide {

Settings LoadSettings();
// False before the first SaveSettings — drives the first-run Welcome.
bool SettingsFileExists();
void SaveSettings(const Settings& settings);
std::wstring SettingsDirectory();
void SetStartWithWindows(bool enabled);
bool IsStartWithWindowsEnabled();

}  // namespace phostide
