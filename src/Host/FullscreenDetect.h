#pragma once

#include <cstdint>
#include <vector>

#include <windows.h>

namespace phostide {

// One top-level window, in z-order (topmost first).
struct ZOrderWindow {
  RECT rc{};
  bool taskbar = false;       // Shell_TrayWnd / Shell_SecondaryTrayWnd
  uint64_t taskbarHash = 0;   // monitor the taskbar sits on (taskbar only)
  bool candidate = false;     // a real app window that may hide a taskbar
};

struct MonitorArea {
  uint64_t deviceHash = 0;
  RECT rc{};
};

// A monitor is fullscreen-covered when a candidate window that spans the whole
// monitor sits above that monitor's taskbar in z-order: the bar cannot be seen,
// whichever window has focus (video left fullscreen on one display while you work
// on another). Writes covered device hashes; returns their count.
uint32_t CoveredMonitorsAboveTaskbar(const std::vector<ZOrderWindow>& zOrder,
                                     const std::vector<MonitorArea>& monitors,
                                     uint64_t* outHashes, uint32_t maxHashes);

// Live desktop: top-level windows via EnumWindows (skips hidden, minimized,
// cloaked, click-through, desktop/shell and this process's own windows).
uint32_t CollectFullscreenCoveredMonitors(uint64_t* outHashes, uint32_t maxHashes);

}  // namespace phostide
