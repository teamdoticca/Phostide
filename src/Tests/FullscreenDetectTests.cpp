// Fullscreen pause: which displays have their taskbar hidden by a fullscreen window.
#include "../Host/FullscreenDetect.cpp"
#include "TestHarness.h"

using namespace phostide;

namespace {

constexpr uint64_t kLeft = 0x11;   // DISPLAY2 at x -2560
constexpr uint64_t kRight = 0x22;  // DISPLAY1 (primary) at x 0

const std::vector<MonitorArea> kMonitors = {
    {kLeft, {-2560, 0, 0, 1440}},
    {kRight, {0, 0, 2560, 1440}},
};

ZOrderWindow App(LONG l, LONG t, LONG r, LONG b) {
  ZOrderWindow w;
  w.rc = {l, t, r, b};
  w.candidate = true;
  return w;
}

ZOrderWindow Bar(uint64_t hash, LONG l, LONG r) {
  ZOrderWindow w;
  w.rc = {l, 1392, r, 1440};
  w.taskbar = true;
  w.taskbarHash = hash;
  return w;
}

std::vector<uint64_t> Covered(const std::vector<ZOrderWindow>& z) {
  uint64_t out[8]{};
  const uint32_t n = CoveredMonitorsAboveTaskbar(z, kMonitors, out, 8);
  return std::vector<uint64_t>(out, out + n);
}

}  // namespace

TEST(Fullscreen_UnfocusedVideoAboveTaskbarCountsOnlyItsDisplay) {
  // Reported case: you work on the left display (focused, z-top), video stays
  // fullscreen on the right one, the right taskbar is far below it.
  const auto covered = Covered({
      App(-2463, 375, -901, 1183),  // focused window on the left display
      Bar(kLeft, -2560, 0),
      App(0, 0, 2560, 1440),  // fullscreen video, not focused
      Bar(kRight, 0, 2560),
  });
  CHECK(covered.size() == 1);
  CHECK(covered.size() == 1 && covered[0] == kRight);
}

TEST(Fullscreen_TaskbarAboveCoveringWindowIsStillVisible) {
  // Maximized-style window that spans the display but sits under its taskbar.
  const auto covered = Covered({Bar(kRight, 0, 2560), App(-8, -8, 2568, 1448)});
  CHECK(covered.empty());
}

TEST(Fullscreen_PartialWindowsNeverCount) {
  const auto covered = Covered({App(0, 0, 2560, 1392), App(100, 100, 900, 900),
                                Bar(kRight, 0, 2560), Bar(kLeft, -2560, 0)});
  CHECK(covered.empty());
}

TEST(Fullscreen_SpanningWindowCoversBothDisplays) {
  const auto covered = Covered({App(-2560, 0, 2560, 1440), Bar(kLeft, -2560, 0),
                                Bar(kRight, 0, 2560)});
  CHECK(covered.size() == 2);
}

TEST(Fullscreen_NonCandidateWindowsAreIgnored) {
  ZOrderWindow overlay = App(0, 0, 2560, 1440);
  overlay.candidate = false;  // e.g. click-through or cloaked
  const auto covered = Covered({overlay, Bar(kRight, 0, 2560)});
  CHECK(covered.empty());
}

TEST(Fullscreen_RespectsOutputCapacity) {
  uint64_t out[1]{};
  const uint32_t n = CoveredMonitorsAboveTaskbar({App(-2560, 0, 2560, 1440)}, kMonitors, out, 1);
  CHECK(n == 1);
  CHECK(CoveredMonitorsAboveTaskbar({App(0, 0, 2560, 1440)}, kMonitors, nullptr, 4) == 0);
}

TEST(CollectFullscreenCoveredMonitors_DoesNotThrow) {
  uint64_t out[8]{};
  CHECK(CollectFullscreenCoveredMonitors(out, 8) <= 8);
}
