#pragma once

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

// Primary taskbar via SHAppBarMessage.
TaskbarGeom QueryPrimaryTaskbarGeom();

// Best-effort match for a BackgroundFill sized w×h (XAML DIPs ≈ px).
TaskbarGeom ResolveTaskbarGeomForSize(int width, int height);

}  // namespace phostide
