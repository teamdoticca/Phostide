#pragma once

#include <vector>

#include <windows.h>

namespace phostide {

enum class TaskbarEdge {
  Bottom = 0,
  Top = 1,
  Left = 2,
  Right = 3,
};

struct TaskbarGeom {
  TaskbarEdge edge = TaskbarEdge::Bottom;
  RECT rc{};
  int length = 1920;     // along the long axis (px)
  int thickness = 48;    // across the short axis (px)
  bool vertical = false;
};

struct TaskbarTray {
  HWND hwnd = nullptr;
  TaskbarGeom geom{};
};

// Primary taskbar via SHAppBarMessage.
TaskbarGeom QueryPrimaryTaskbarGeom();

TaskbarGeom GeomFromTrayHwnd(HWND hwnd);

// Shell_TrayWnd + all Shell_SecondaryTrayWnd.
std::vector<TaskbarTray> EnumerateTaskbarTrays();

// Best-effort match for a BackgroundFill sized w×h (XAML DIPs ≈ px).
// skippedHwnds: trays already claimed by another visual (same-size monitors).
TaskbarGeom ResolveTaskbarGeomForSize(int width, int height,
                                      const HWND* skippedHwnds = nullptr,
                                      size_t skippedCount = 0,
                                      HWND* outHwnd = nullptr);

}  // namespace phostide
