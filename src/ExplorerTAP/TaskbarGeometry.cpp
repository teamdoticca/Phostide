#include "TaskbarGeometry.h"

#include <algorithm>
#include <climits>
#include <cmath>

#include <shellapi.h>

namespace phostide {
namespace {

TaskbarEdge EdgeFromAppBar(UINT uEdge) {
  switch (uEdge) {
    case ABE_TOP: return TaskbarEdge::Top;
    case ABE_LEFT: return TaskbarEdge::Left;
    case ABE_RIGHT: return TaskbarEdge::Right;
    case ABE_BOTTOM:
    default: return TaskbarEdge::Bottom;
  }
}

TaskbarGeom FromRect(RECT rc, TaskbarEdge edgeHint) {
  TaskbarGeom g;
  g.rc = rc;
  const int w = (std::max)(1, static_cast<int>(rc.right - rc.left));
  const int h = (std::max)(1, static_cast<int>(rc.bottom - rc.top));
  g.vertical = h > w;
  if (g.vertical) {
    g.length = h;
    g.thickness = w;
    if (edgeHint == TaskbarEdge::Left || edgeHint == TaskbarEdge::Right) {
      g.edge = edgeHint;
    } else {
      g.edge = (rc.left <= 8) ? TaskbarEdge::Left : TaskbarEdge::Right;
    }
  } else {
    g.length = w;
    g.thickness = h;
    if (edgeHint == TaskbarEdge::Top || edgeHint == TaskbarEdge::Bottom) {
      g.edge = edgeHint;
    } else {
      g.edge = (rc.top <= 8) ? TaskbarEdge::Top : TaskbarEdge::Bottom;
    }
  }
  return g;
}

int SizeScore(const TaskbarGeom& g, int width, int height) {
  const int dw = std::abs((g.rc.right - g.rc.left) - width);
  const int dh = std::abs((g.rc.bottom - g.rc.top) - height);
  return dw + dh;
}

bool IsSkipped(HWND hwnd, const HWND* skippedHwnds, size_t skippedCount) {
  if (!hwnd || !skippedHwnds || skippedCount == 0) return false;
  for (size_t i = 0; i < skippedCount; ++i) {
    if (skippedHwnds[i] == hwnd) return true;
  }
  return false;
}

}  // namespace

TaskbarGeom QueryPrimaryTaskbarGeom() {
  APPBARDATA abd{};
  abd.cbSize = sizeof(abd);
  if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
    return FromRect(abd.rc, EdgeFromAppBar(abd.uEdge));
  }
  if (HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    return GeomFromTrayHwnd(tray);
  }
  TaskbarGeom fallback;
  fallback.length = GetSystemMetrics(SM_CXSCREEN);
  fallback.thickness = 48;
  fallback.edge = TaskbarEdge::Bottom;
  fallback.rc = {0, GetSystemMetrics(SM_CYSCREEN) - 48, fallback.length,
                 GetSystemMetrics(SM_CYSCREEN)};
  return fallback;
}

TaskbarGeom GeomFromTrayHwnd(HWND hwnd) {
  RECT rc{};
  if (!hwnd || !GetWindowRect(hwnd, &rc)) {
    return QueryPrimaryTaskbarGeom();
  }
  TaskbarEdge hint = TaskbarEdge::Bottom;
  wchar_t cls[64]{};
  if (GetClassNameW(hwnd, cls, 64) && _wcsicmp(cls, L"Shell_TrayWnd") == 0) {
    APPBARDATA abd{};
    abd.cbSize = sizeof(abd);
    if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
      hint = EdgeFromAppBar(abd.uEdge);
      // Prefer AppBar rect for primary — more accurate than the outer HWND.
      return FromRect(abd.rc, hint);
    }
  }
  return FromRect(rc, hint);
}

std::vector<TaskbarTray> EnumerateTaskbarTrays() {
  std::vector<TaskbarTray> out;
  if (HWND primary = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    out.push_back(TaskbarTray{primary, GeomFromTrayHwnd(primary)});
  }
  HWND secondary = nullptr;
  while ((secondary = FindWindowExW(nullptr, secondary, L"Shell_SecondaryTrayWnd", nullptr))) {
    out.push_back(TaskbarTray{secondary, GeomFromTrayHwnd(secondary)});
  }
  return out;
}

TaskbarGeom ResolveTaskbarGeomForSize(int width, int height, const HWND* skippedHwnds,
                                      size_t skippedCount, HWND* outHwnd) {
  if (outHwnd) *outHwnd = nullptr;

  const auto trays = EnumerateTaskbarTrays();
  TaskbarGeom best{};
  HWND bestHwnd = nullptr;
  int bestScore = INT_MAX;
  bool found = false;

  for (const auto& tray : trays) {
    if (!tray.hwnd || IsSkipped(tray.hwnd, skippedHwnds, skippedCount)) continue;
    const int score = SizeScore(tray.geom, width, height);
    // Prefer unused trays even on a score tie (identical monitor widths).
    if (!found || score < bestScore) {
      found = true;
      bestScore = score;
      best = tray.geom;
      bestHwnd = tray.hwnd;
    }
  }

  if (!found) {
    best = QueryPrimaryTaskbarGeom();
    bestHwnd = FindWindowW(L"Shell_TrayWnd", nullptr);
  }

  if (height > width * 3 / 2) {
    best.vertical = true;
    best.length = (std::max)(height, best.length);
    best.thickness = (std::max)(1, width);
    if (best.edge != TaskbarEdge::Left && best.edge != TaskbarEdge::Right) {
      best.edge = TaskbarEdge::Left;
    }
  } else if (width > height * 3 / 2) {
    best.vertical = false;
    best.length = (std::max)(width, best.length);
    best.thickness = (std::max)(1, height);
    if (best.edge != TaskbarEdge::Top && best.edge != TaskbarEdge::Bottom) {
      best.edge = TaskbarEdge::Bottom;
    }
  }

  if (outHwnd) *outHwnd = bestHwnd;
  return best;
}

}  // namespace phostide
