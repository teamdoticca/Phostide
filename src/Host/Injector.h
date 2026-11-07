#pragma once

#include <string>

#include <windows.h>

namespace phostide {

// Copies ExplorerTAP.dll next to a writable temp path and loads it into the
// taskbar thread via WH_CALLWNDPROC (same family of technique as TranslucentTB).
bool InjectExplorerTap(std::wstring* errorOut = nullptr);
bool SendThemeCommand(const struct ThemeCommand& cmd);
std::wstring TapDllPath();

}  // namespace phostide
