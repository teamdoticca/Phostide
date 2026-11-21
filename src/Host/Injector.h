#pragma once

#include <string>

#include <windows.h>

namespace phostide {

// Copies ExplorerTAP.dll next to a writable temp path and loads it into the
// taskbar thread via WH_CALLWNDPROC (same family of technique as TranslucentTB).
// reapply: current host command, re-sent if a TAP already loaded in this Explorer
// has not signaled Bound — avoids an Explorer restart on host relaunch.
bool InjectExplorerTap(std::wstring* errorOut = nullptr,
                       const struct ThemeCommand* reapply = nullptr);
// Tell TAP to restore the stock taskbar fill (enabled=0).
bool RequestTapRestore();
// Clear Local\Phostide.TAP.Bound after Explorer recreates the tray.
void ResetTapBoundEvent();
// Drop WH_CALLWNDPROC + FreeLibrary in the host so upgrades can replace files.
void DetachExplorerTap();
// Host shutdown: if an Explorer restart is in flight on the inject worker, make
// sure Explorer is started again before the process exits.
void FinishPendingExplorerRestart();
// Ask a running PhostideTray instance to exit; wait until its mutex clears.
bool RequestExistingInstanceExit(DWORD timeoutMs = 20000);
bool SendThemeCommand(const struct ThemeCommand& cmd);
std::wstring TapDllPath();

}  // namespace phostide
