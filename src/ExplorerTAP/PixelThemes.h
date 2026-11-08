#pragma once

#include "TaskbarGeometry.h"
#include "../Common/PhostideCommon.h"

#include <cstdint>

namespace phostide {

struct PaintContext {
  ThemeId theme = ThemeId::Rainbow;
  float intensity = 0.45f;
  float timeSeconds = 0.f;
  TaskbarEdge edge = TaskbarEdge::Bottom;
  bool vertical = false;
  int thickness = 48;  // short-axis px (from SHAppBar / XAML size)
  int length = 1920;   // long-axis px
};

// CPU BGRA8 theme painters for WriteableBitmap (bitmap is shape width×height).
void PaintThemePixels(const PaintContext& ctx, uint8_t* bgra, int width, int height,
                      int strideBytes);

}  // namespace phostide
