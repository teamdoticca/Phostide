#pragma once

#include "../Common/PhostideCommon.h"

namespace phostide {

Settings LoadSettings();
void SaveSettings(const Settings& settings);
std::wstring SettingsDirectory();
void SetStartWithWindows(bool enabled);
bool IsStartWithWindowsEnabled();

}  // namespace phostide
