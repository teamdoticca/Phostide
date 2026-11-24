#include "FullscreenDetect.h"

#include "../Common/MonitorEnum.h"

#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")

namespace phostide {
namespace {

constexpr LONG kCoverTolerancePx = 4;

bool CoversMonitor(const RECT& wr, const RECT& mr) {
  const LONG tol = kCoverTolerancePx;
  return wr.left <= mr.left + tol && wr.top <= mr.top + tol &&
         wr.right >= mr.right - tol && wr.bottom >= mr.bottom - tol;
}

bool Contains(const uint64_t* hashes, uint32_t count, uint64_t hash) {
  for (uint32_t i = 0; i < count; ++i) {
    if (hashes[i] == hash) return true;
  }
  return false;
}

bool ClassIs(const wchar_t* cls, const wchar_t* name) { return _wcsicmp(cls, name) == 0; }

}  // namespace

uint32_t CoveredMonitorsAboveTaskbar(const std::vector<ZOrderWindow>& zOrder,
                                     const std::vector<MonitorArea>& monitors,
                                     uint64_t* outHashes, uint32_t maxHashes) {
  if (!outHashes || maxHashes == 0) return 0;
  std::vector<uint64_t> taskbarSeen;
  uint32_t count = 0;
  for (const auto& w : zOrder) {
    if (w.taskbar) {
      if (w.taskbarHash) taskbarSeen.push_back(w.taskbarHash);
      continue;
    }
    if (!w.candidate) continue;
    for (const auto& mon : monitors) {
      if (count >= maxHashes) return count;
      if (!mon.deviceHash || Contains(outHashes, count, mon.deviceHash)) continue;
      if (Contains(taskbarSeen.data(), static_cast<uint32_t>(taskbarSeen.size()),
                   mon.deviceHash)) {
        continue;  // that bar is above this window: still visible
      }
      if (CoversMonitor(w.rc, mon.rc)) outHashes[count++] = mon.deviceHash;
    }
  }
  return count;
}

uint32_t CollectFullscreenCoveredMonitors(uint64_t* outHashes, uint32_t maxHashes) {
  std::vector<MonitorArea> monitors;
  for (const auto& mon : EnumerateAttachedMonitors()) monitors.push_back({mon.deviceHash, mon.rc});
  if (monitors.empty()) return 0;

  struct Ctx {
    std::vector<ZOrderWindow> zOrder;
    DWORD selfPid = GetCurrentProcessId();
  } ctx;

  EnumWindows(
      [](HWND hwnd, LPARAM lp) -> BOOL {
        auto& c = *reinterpret_cast<Ctx*>(lp);
        if (!IsWindowVisible(hwnd)) return TRUE;
        wchar_t cls[64]{};
        GetClassNameW(hwnd, cls, 64);
        ZOrderWindow w;
        if (!GetWindowRect(hwnd, &w.rc)) return TRUE;
        if (ClassIs(cls, L"Shell_TrayWnd") || ClassIs(cls, L"Shell_SecondaryTrayWnd")) {
          w.taskbar = true;
          DeviceHashFromWindow(hwnd, w.taskbarHash);
          c.zOrder.push_back(w);
          return TRUE;
        }
        if (IsIconic(hwnd)) return TRUE;
        if (ClassIs(cls, L"Progman") || ClassIs(cls, L"WorkerW")) return TRUE;  // desktop
        DWORD pid = 0;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == c.selfPid) return TRUE;  // Settings, Identify cards, tray window
        if (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TRANSPARENT) return TRUE;  // click-through overlay
        DWORD cloaked = 0;
        if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) &&
            cloaked) {
          return TRUE;  // other virtual desktop, suspended UWP, hidden shell surfaces
        }
        w.candidate = true;
        c.zOrder.push_back(w);
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&ctx));

  return CoveredMonitorsAboveTaskbar(ctx.zOrder, monitors, outHashes, maxHashes);
}

}  // namespace phostide
