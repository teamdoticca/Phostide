#include "TaskbarGeometry.h"

#include <algorithm>
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
      // Infer from screen position.
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

}  // namespace

TaskbarGeom QueryPrimaryTaskbarGeom() {
  APPBARDATA abd{};
  abd.cbSize = sizeof(abd);
  if (SHAppBarMessage(ABM_GETTASKBARPOS, &abd)) {
    return FromRect(abd.rc, EdgeFromAppBar(abd.uEdge));
  }
  if (HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr)) {
    RECT rc{};
    if (GetWindowRect(tray, &rc)) {
      return FromRect(rc, TaskbarEdge::Bottom);
    }
  }
  TaskbarGeom fallback;
  fallback.length = GetSystemMetrics(SM_CXSCREEN);
  fallback.thickness = 48;
  fallback.edge = TaskbarEdge::Bottom;
  fallback.rc = {0, GetSystemMetrics(SM_CYSCREEN) - 48, fallback.length, GetSystemMetrics(SM_CYSCREEN)};
  return fallback;
}

TaskbarGeom ResolveTaskbarGeomForSize(int width, int height) {
  TaskbarGeom best = QueryPrimaryTaskbarGeom();
  int bestScore = SizeScore(best, width, height);

  auto consider = [&](HWND hwnd, TaskbarEdge hint) {
    if (!hwnd) return;
    RECT rc{};
    if (!GetWindowRect(hwnd, &rc)) return;
    TaskbarGeom g = FromRect(rc, hint);
    const int score = SizeScore(g, width, height);
    if (score < bestScore) {
      bestScore = score;
      best = g;
    }
  };

  consider(FindWindowW(L"Shell_TrayWnd", nullptr), best.edge);
  HWND secondary = nullptr;
  while ((secondary = FindWindowExW(nullptr, secondary, L"Shell_SecondaryTrayWnd", nullptr))) {
    // Infer edge from rect vs monitor work area later; size match is enough for now.
    consider(secondary, TaskbarEdge::Bottom);
  }

  // If XAML size strongly disagrees with matched window, trust aspect ratio.
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

  return best;
}

}  // namespace phostide
