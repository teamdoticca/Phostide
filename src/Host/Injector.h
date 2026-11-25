#pragma once

#include <cstdint>
#include <string>

#include <windows.h>

namespace phostide {

// XAML Diagnostics in a freshly started Explorer returns ERROR_NOT_FOUND while the
// taskbar island is still warming, and that failure sticks for the process, so the
// TAP is only hooked once Explorer is this old (Setup's own shell restart settles
// 8 s and Start with Windows runs at logon, both too early).
inline constexpr uint64_t kExplorerSettleMs = 15000;
// How long to wait before hooking an Explorer that has run for explorerAgeMs
// (UINT64_MAX = unknown age: do not wait).
uint32_t ExplorerSettleWaitMs(uint64_t explorerAgeMs);

// Copies ExplorerTAP.dll next to a writable temp path and loads it into the
// taskbar thread via WH_CALLWNDPROC (same family of technique as TranslucentTB).
// reapply: current host command, handed to the TAP right after the hook (a fresh
// TAP binds nothing until it arrives). With paint off / paused, Bound is not
// required, so starting paused never restarts Explorer.
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
