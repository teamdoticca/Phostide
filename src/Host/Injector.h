#pragma once

#include <string>

#include <windows.h>

namespace phostide {

// Copies ExplorerTAP.dll next to a writable temp path and loads it into the
// taskbar thread via WH_CALLWNDPROC (same family of technique as TranslucentTB).
bool InjectExplorerTap(std::wstring* errorOut = nullptr);
// Tell TAP to restore the stock taskbar fill (enabled=0).
bool RequestTapRestore();
// Drop WH_CALLWNDPROC + FreeLibrary in the host so upgrades can replace files.
void DetachExplorerTap();
// Ask a running PhostideTray instance to exit; wait until its mutex clears.
bool RequestExistingInstanceExit(DWORD timeoutMs = 20000);
bool SendThemeCommand(const struct ThemeCommand& cmd);
std::wstring TapDllPath();

}  // namespace phostide
