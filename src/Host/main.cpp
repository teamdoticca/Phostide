#include "Injector.h"
#include "SettingsStore.h"
#include "SettingsWindow.h"
#include "UpdateCheck.h"
#include "resource.h"
#include "../Common/PhostideCommon.h"
#include "../Common/MonitorEnum.h"
#include "../Common/TaskbarAccent.h"

#include <shellapi.h>
#include <shobjidl.h>
#include <windows.h>
#include <wtsapi32.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <memory>
#include <string>
#include <thread>

namespace {

constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT WM_INJECT_DONE = WM_APP + 2;
constexpr UINT WM_UPDATE_DONE = WM_APP + 3;
constexpr UINT_PTR ID_ANIM_TIMER = 1;
constexpr UINT_PTR ID_REINJECT_TIMER = 2;
constexpr UINT_PTR ID_IDLE_TIMER = 3;
constexpr UINT ID_ENABLE = 100;
constexpr UINT ID_SETTINGS = 110;
constexpr UINT ID_OPEN_UPDATE = 310;
constexpr UINT ID_EXIT = 399;
constexpr UINT kAnimIntervalMs = 33;  // ~30 fps host-side accent animation
constexpr UINT kReinjectDebounceMs = 1000;
constexpr UINT kIdlePollMs = 1000;       // while awake — cheap attendance
constexpr UINT kIdlePollCareMs = 200;    // while any bar is dimmed — snappy wake
constexpr UINT kFullscreenEnterDwellMs = 750;
constexpr UINT kFullscreenLeaveDwellMs = 300;
constexpr LONG kFullscreenCoverTolerancePx = 4;

struct MonitorCareState {
  uint64_t deviceHash = 0;
  DWORD lastAttendedTick = 0;
  bool idle = false;
  bool primary = false;
};

HWND g_hwnd = nullptr;
HINSTANCE g_instance = nullptr;
NOTIFYICONDATAW g_nid{};
phostide::Settings g_settings{};
HMENU g_menu = nullptr;
bool g_trayAdded = false;
bool g_trayIconOwned = false;
UINT g_taskbarCreatedMsg = 0;
bool g_sessionLocked = false;
bool g_fullscreenPaused = false;
bool g_fullscreenRaw = false;
DWORD g_fullscreenEdgeTick = 0;
uint64_t g_fullscreenHashes[phostide::kMaxMonitorSlots]{};
uint32_t g_fullscreenHashCount = 0;
MonitorCareState g_careMonitors[phostide::kMaxMonitorSlots]{};
uint32_t g_careMonitorCount = 0;
DWORD g_careProcessStartTick = 0;
bool g_careWasSystemIdle = false;
bool g_tapBound = false;
bool g_clearedAccentForTap = false;
bool g_updateCheckRunning = false;
std::atomic<bool> g_injectInFlight{false};
std::wstring g_updateStatusText;
std::chrono::steady_clock::time_point g_animStart = std::chrono::steady_clock::now();
HANDLE g_boundEvent = nullptr;

struct UpdateDonePayload {
  bool manual = false;
  phostide::UpdateCheckResult result;
};

void ApplyTheme(bool pushToTap);
void SyncAnimTimer(HWND hwnd);
void ShowBalloon(const wchar_t* title, const wchar_t* body);

bool IsNightBoostWindow() {
  SYSTEMTIME st{};
  GetLocalTime(&st);
  // 20:00–06:59 — quieter OLED stretch for evening / overnight desks.
  return st.wHour >= 20 || st.wHour < 7;
}

int EffectiveCareIdleMinutes() {
  int minutes = phostide::ClampOledCareIdleMinutes(g_settings.oledCareIdleMinutes);
  if (minutes <= 0) return 0;
  if (g_settings.oledCareNightBoost && IsNightBoostWindow()) {
    minutes = (std::max)(1, minutes / 2);
  }
  return minutes;
}

int EffectiveCareFillIntensity() {
  int fill = phostide::ClampPercent(g_settings.oledCareFillIntensity);
  if (g_settings.oledCareNightBoost && IsNightBoostWindow()) {
    fill = (std::max)(0, fill - 5);
  }
  return fill;
}

uint32_t CareScaledIntensity(int baseIntensity, bool idle) {
  if (!idle) return static_cast<uint32_t>(phostide::ClampPercent(baseIntensity));
  // Visible drop: honor Idle fill %, but never stay above ~20% of the live intensity.
  const int care = EffectiveCareFillIntensity();
  const int capped = (std::max)(2, phostide::ClampPercent(baseIntensity) / 5);
  return static_cast<uint32_t>((std::min)(care, capped));
}

bool MonitorCareIdle(uint64_t deviceHash) {
  if (!deviceHash) return false;
  for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
    if (g_careMonitors[i].deviceHash == deviceHash) return g_careMonitors[i].idle;
  }
  return false;
}

void ClearMonitorCareIdle() {
  for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
    g_careMonitors[i].idle = false;
  }
}

bool IsShellOrOwnWindow(HWND hwnd) {
  if (!hwnd) return true;
  if (hwnd == g_hwnd) return true;
  HWND settings = phostide::SettingsWindowHwnd();
  if (settings && (hwnd == settings || IsChild(settings, hwnd))) return true;
  wchar_t cls[64]{};
  if (GetClassNameW(hwnd, cls, 64) <= 0) return false;
  return _wcsicmp(cls, L"Progman") == 0 || _wcsicmp(cls, L"WorkerW") == 0 ||
         _wcsicmp(cls, L"Shell_TrayWnd") == 0 ||
         _wcsicmp(cls, L"Shell_SecondaryTrayWnd") == 0;
}

bool WindowCoversMonitor(const RECT& wr, const RECT& mr) {
  const LONG tol = kFullscreenCoverTolerancePx;
  return wr.left <= mr.left + tol && wr.top <= mr.top + tol &&
         wr.right >= mr.right - tol && wr.bottom >= mr.bottom - tol;
}

uint32_t CollectFullscreenCoveredHashes(uint64_t* outHashes, uint32_t maxHashes) {
  if (!outHashes || maxHashes == 0) return 0;
  HWND fg = GetForegroundWindow();
  if (!fg || !IsWindowVisible(fg) || IsIconic(fg)) return 0;
  if (IsShellOrOwnWindow(fg)) return 0;

  RECT wr{};
  if (!GetWindowRect(fg, &wr)) return 0;

  uint32_t count = 0;
  for (const auto& mon : phostide::EnumerateAttachedMonitors()) {
    if (count >= maxHashes) break;
    if (!WindowCoversMonitor(wr, mon.rc)) continue;
    outHashes[count++] = mon.deviceHash;
  }
  return count;
}

bool FullscreenHashSetsEqual(const uint64_t* a, uint32_t aCount, const uint64_t* b,
                             uint32_t bCount) {
  if (aCount != bCount) return false;
  for (uint32_t i = 0; i < aCount; ++i) {
    bool found = false;
    for (uint32_t j = 0; j < bCount; ++j) {
      if (a[i] == b[j]) {
        found = true;
        break;
      }
    }
    if (!found) return false;
  }
  return true;
}

bool IsFullscreenHash(uint64_t deviceHash) {
  if (!deviceHash) return false;
  for (uint32_t i = 0; i < g_fullscreenHashCount; ++i) {
    if (g_fullscreenHashes[i] == deviceHash) return true;
  }
  return false;
}

bool RefreshFullscreenPauseState() {
  if (!g_settings.pauseWhenFullscreen || !g_settings.enabled) {
    if (!g_fullscreenPaused && !g_fullscreenRaw && g_fullscreenHashCount == 0) return false;
    g_fullscreenPaused = false;
    g_fullscreenRaw = false;
    g_fullscreenEdgeTick = 0;
    g_fullscreenHashCount = 0;
    return true;
  }

  uint64_t covered[phostide::kMaxMonitorSlots]{};
  const uint32_t coveredCount =
      CollectFullscreenCoveredHashes(covered, phostide::kMaxMonitorSlots);
  const bool raw = coveredCount > 0;
  const DWORD now = GetTickCount();
  if (raw != g_fullscreenRaw) {
    g_fullscreenRaw = raw;
    g_fullscreenEdgeTick = now;
  }

  const DWORD dwell = g_fullscreenRaw ? kFullscreenEnterDwellMs : kFullscreenLeaveDwellMs;
  const bool settled = (now - g_fullscreenEdgeTick) >= dwell;
  const bool want = settled ? g_fullscreenRaw : g_fullscreenPaused;

  bool changed = false;
  if (want != g_fullscreenPaused) {
    g_fullscreenPaused = want;
    changed = true;
  }

  if (g_fullscreenPaused) {
    // While paused, keep the covered-monitor set current (Alt+Tab between games).
    if (!FullscreenHashSetsEqual(g_fullscreenHashes, g_fullscreenHashCount, covered,
                                 coveredCount)) {
      g_fullscreenHashCount = coveredCount;
      for (uint32_t i = 0; i < coveredCount; ++i) g_fullscreenHashes[i] = covered[i];
      changed = true;
    }
  } else if (g_fullscreenHashCount != 0) {
    g_fullscreenHashCount = 0;
    changed = true;
  }
  return changed;
}

bool IsThemePaintPaused() {
  // Host accent fallback: lock freezes all; fullscreen freezes when any bar is covered.
  return (g_settings.pauseWhenLocked && g_sessionLocked) ||
         (g_settings.pauseWhenFullscreen && g_fullscreenPaused);
}

void TouchCareMonitor(uint64_t deviceHash, DWORD now) {
  if (!deviceHash) return;
  for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
    if (g_careMonitors[i].deviceHash != deviceHash) continue;
    g_careMonitors[i].lastAttendedTick = now;
    return;
  }
}

// Returns true if any monitor's idle bit changed.
bool RefreshOledCareIdleStates() {
  const int minutes = EffectiveCareIdleMinutes();
  const DWORD now = GetTickCount();
  if (g_careProcessStartTick == 0) g_careProcessStartTick = now;
  const auto monitors = phostide::EnumerateAttachedMonitors();

  // Rebuild attendance table when the monitor set changes; keep lastAttended for survivors.
  MonitorCareState next[phostide::kMaxMonitorSlots]{};
  uint32_t nextCount = 0;
  for (const auto& mon : monitors) {
    if (nextCount >= phostide::kMaxMonitorSlots) break;
    MonitorCareState st;
    st.deviceHash = mon.deviceHash;
    st.primary = mon.primary;
    st.lastAttendedTick = now;
    st.idle = false;
    for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
      if (g_careMonitors[i].deviceHash == mon.deviceHash) {
        st.lastAttendedTick = g_careMonitors[i].lastAttendedTick;
        st.idle = g_careMonitors[i].idle;
        break;
      }
    }
    next[nextCount++] = st;
  }
  std::memcpy(g_careMonitors, next, sizeof(next));
  g_careMonitorCount = nextCount;

  if (minutes <= 0 || !g_settings.enabled || g_sessionLocked || g_careMonitorCount == 0) {
    bool changed = false;
    for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
      if (g_careMonitors[i].idle) {
        g_careMonitors[i].idle = false;
        changed = true;
      }
    }
    return changed;
  }

  const DWORD thresholdMs = static_cast<DWORD>(minutes) * 60u * 1000u;

  // Startup grace: Explorer reinject / settle often leaves GetLastInputInfo already
  // "old", which would enter care immediately and feel stuck until input.
  constexpr DWORD kStartupGraceMs = 8000;
  if (now - g_careProcessStartTick < kStartupGraceMs) {
    bool changed = false;
    for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
      g_careMonitors[i].lastAttendedTick = now;
      if (g_careMonitors[i].idle) {
        g_careMonitors[i].idle = false;
        changed = true;
      }
    }
    return changed;
  }

  LASTINPUTINFO lii{sizeof(lii)};
  const bool haveInput = GetLastInputInfo(&lii) != FALSE;
  const bool systemIdle = haveInput && (now - lii.dwTime) >= thresholdMs;

  if (systemIdle) {
    g_careWasSystemIdle = true;
  } else {
    // Per-monitor attendance follows the cursor only. Touching the focused-window
    // monitor every poll kept a leftover FG on the secondary awake forever while
    // you worked on the primary — split dim never stuck. Full AFK still uses
    // systemIdle (all bars). Wake-all when leaving AFK stays below.
    POINT cursor{};
    if (GetCursorPos(&cursor)) {
      uint64_t hash = 0;
      if (phostide::DeviceHashFromHmonitor(
              MonitorFromPoint(cursor, MONITOR_DEFAULTTONEAREST), hash)) {
        TouchCareMonitor(hash, now);
      }
    }
    if (g_careWasSystemIdle) {
      for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
        g_careMonitors[i].lastAttendedTick = now;
      }
      g_careWasSystemIdle = false;
    }
  }

  bool changed = false;
  uint32_t idleCount = 0;
  for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
    const bool unattended =
        (now - g_careMonitors[i].lastAttendedTick) >= thresholdMs;
    const bool idle = systemIdle || unattended;
    if (idle != g_careMonitors[i].idle) {
      g_careMonitors[i].idle = idle;
      changed = true;
    }
    if (g_careMonitors[i].idle) ++idleCount;
  }
  if (changed && idleCount > 0) {
    wchar_t body[80];
    swprintf_s(body, L"OLED Care on — %u display(s) dimmed.", idleCount);
    ShowBalloon(L"Phostide", body);
  }
  return changed;
}

phostide::ThemeCommand MakeCommand() {
  phostide::ThemeCommand cmd;
  cmd.apiVersion = phostide::kTapApiVersion;
  cmd.theme = static_cast<uint32_t>(g_settings.theme);
  cmd.enabled = g_settings.enabled ? 1u : 0u;
  // Global paused = session lock only. Fullscreen uses per-slot pauseActive (v7).
  cmd.paused =
      (g_settings.enabled && g_settings.pauseWhenLocked && g_sessionLocked) ? 1u : 0u;
  cmd.renderMode = static_cast<uint32_t>(g_settings.renderMode);
  cmd.chromeOpacity =
      static_cast<uint32_t>(phostide::ClampPercent(g_settings.oledCareChromeOpacity));
  cmd.clockOpacity =
      static_cast<uint32_t>(phostide::ClampPercent(g_settings.oledCareClockOpacity));
  cmd.careFlags = 0;
  if (g_settings.oledCareChromeDim) cmd.careFlags |= phostide::kCareFlagChromeDim;
  if (g_settings.oledCareClockNudge) cmd.careFlags |= phostide::kCareFlagClockNudge;

  const auto monitors = phostide::EnumerateAttachedMonitors();
  cmd.slotCount = 0;
  bool anyCare = false;
  bool primaryIdle = false;
  bool sawPrimary = false;
  for (const auto& mon : monitors) {
    if (cmd.slotCount >= phostide::kMaxMonitorSlots) break;
    const auto appearance =
        phostide::ResolveMonitorAppearance(g_settings, mon.deviceName);
    const bool idle = MonitorCareIdle(mon.deviceHash);
    auto& slot = cmd.slots[cmd.slotCount++];
    slot.deviceHash = mon.deviceHash;
    slot.theme = static_cast<uint32_t>(appearance.theme);
    slot.intensity = CareScaledIntensity(appearance.intensity, idle);
    slot.enabled =
        phostide::IsMonitorDisabled(g_settings, mon.deviceName) ? 0u : 1u;
    const bool fsPause = g_settings.pauseWhenFullscreen && IsFullscreenHash(mon.deviceHash);
    slot.pauseActive = (fsPause && slot.enabled && g_settings.enabled) ? 1u : 0u;
    slot.careActive =
        (idle && slot.enabled && g_settings.enabled && !g_sessionLocked && !fsPause)
            ? 1u
            : 0u;
    if (slot.careActive) anyCare = true;
    if (mon.primary) {
      sawPrimary = true;
      primaryIdle = idle;
    }
  }
  cmd.careActive = anyCare ? 1u : 0u;
  cmd.intensity = CareScaledIntensity(g_settings.intensity, sawPrimary ? primaryIdle : anyCare);
  return cmd;
}

void SyncIdleTimer(HWND hwnd) {
  const bool careOn = g_settings.oledCareIdleMinutes > 0 && g_settings.enabled;
  const bool fsOn = g_settings.pauseWhenFullscreen && g_settings.enabled;
  if (!careOn && !fsOn) {
    KillTimer(hwnd, ID_IDLE_TIMER);
    ClearMonitorCareIdle();
    g_fullscreenPaused = false;
    g_fullscreenRaw = false;
    g_fullscreenEdgeTick = 0;
    g_fullscreenHashCount = 0;
    return;
  }
  if (!careOn) ClearMonitorCareIdle();

  bool anyIdle = false;
  if (careOn) {
    for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
      if (g_careMonitors[i].idle) {
        anyIdle = true;
        break;
      }
    }
  }
  // Faster while dimmed or fullscreen-paused — snappy wake / Alt+Tab resume.
  const bool fast = anyIdle || g_fullscreenPaused;
  SetTimer(hwnd, ID_IDLE_TIMER, fast ? kIdlePollCareMs : kIdlePollMs, nullptr);
}

void PollOledCareIdle(HWND hwnd) {
  bool changed = false;
  if (g_settings.oledCareIdleMinutes > 0) {
    changed = RefreshOledCareIdleStates();
  }
  if (RefreshFullscreenPauseState()) changed = true;
  // Re-arm so wake / FS leave uses the fast interval while active.
  SyncIdleTimer(hwnd);
  if (phostide::SettingsWindowIsOpen()) {
    phostide::SettingsWindowRefreshPauseUi();
  }
  if (!changed) return;
  SyncAnimTimer(hwnd);
  ApplyTheme(true);
}

float AnimSeconds() {
  using namespace std::chrono;
  return duration<float>(steady_clock::now() - g_animStart).count();
}

bool RefreshTapBound() {
  // Always re-query — TAP ClearBoundEvent on suspend/Exit must not leave a sticky true.
  if (g_boundEvent && WaitForSingleObject(g_boundEvent, 0) == WAIT_OBJECT_0) {
    g_tapBound = true;
  } else {
    g_tapBound = false;
  }
  return g_tapBound;
}

void StartInjectWorker() {
  bool expected = false;
  if (!g_injectInFlight.compare_exchange_strong(expected, true)) {
    return;
  }
  std::thread([] {
    std::wstring err;
    phostide::InjectExplorerTap(&err);
    if (g_hwnd) {
      PostMessageW(g_hwnd, WM_INJECT_DONE, 0, 0);
    } else {
      g_injectInFlight.store(false);
    }
  }).detach();
}

void ApplyTheme(bool pushToTap) {
  const auto cmd = MakeCommand();
  const bool bound = RefreshTapBound();
  if (bound) {
    // XAML BackgroundFill owns pixels — drop accent to avoid double-tint.
    if (!g_clearedAccentForTap) {
      phostide::ClearTaskbarAccent();
      g_clearedAccentForTap = true;
    }
  } else {
    g_clearedAccentForTap = false;
    phostide::ApplyTaskbarAccent(cmd, AnimSeconds());
  }
  if (pushToTap) {
    phostide::SendThemeCommand(cmd);
  }
}

void SyncAnimTimer(HWND hwnd) {
  // Accent animation only while TAP has not bound real BackgroundFill pixels.
  const bool run =
      g_settings.enabled && !IsThemePaintPaused() && !RefreshTapBound();
  if (run) {
    SetTimer(hwnd, ID_ANIM_TIMER, kAnimIntervalMs, nullptr);
  } else {
    KillTimer(hwnd, ID_ANIM_TIMER);
  }
}

void ScheduleReinjectAfterTrayRecreate(HWND hwnd) {
  // Explorer recreated the tray — TAP is gone or painting dead XAML.
  g_tapBound = false;
  g_clearedAccentForTap = false;
  phostide::ResetTapBoundEvent();
  // Same-process rebuild: drop stale fills so the next Apply triggers a rescan.
  phostide::RequestTapRestore();
  SyncAnimTimer(hwnd);
  ApplyTheme(true);
  // Debounce bursts of TaskbarCreated (and overlap with our own shell bounce).
  SetTimer(hwnd, ID_REINJECT_TIMER, kReinjectDebounceMs, nullptr);
}

bool HasPendingUpdate() {
  return !g_settings.pendingUpdateTag.empty() &&
         phostide::IsVersionNewer(g_settings.pendingUpdateTag, phostide::kAppVersion);
}

void ShowBalloon(const wchar_t* title, const wchar_t* body) {
  if (!g_trayAdded) return;
  NOTIFYICONDATAW tip = g_nid;
  tip.uFlags = NIF_INFO | NIF_GUID;
  tip.guidItem = phostide::kTrayIconGuid;
  tip.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
  wcsncpy_s(tip.szInfoTitle, title, _TRUNCATE);
  wcsncpy_s(tip.szInfo, body, _TRUNCATE);
  Shell_NotifyIconW(NIM_MODIFY, &tip);
}

void OpenPendingUpdate() {
  const wchar_t* url = nullptr;
  if (!g_settings.pendingSetupUrl.empty()) {
    url = g_settings.pendingSetupUrl.c_str();
  } else {
    url = L"https://github.com/teamdoticca/Phostide/releases/latest";
  }
  ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

void OnApplyAppearanceFromSettings() {
  g_animStart = std::chrono::steady_clock::now();
  phostide::SaveSettings(g_settings);
  SyncIdleTimer(g_hwnd);
  RefreshOledCareIdleStates();
  RefreshFullscreenPauseState();
  SyncAnimTimer(g_hwnd);
  ApplyTheme(true);
  phostide::SettingsWindowRefreshPauseUi();
}

void StartUpdateCheck(bool manual);

void OnRequestUpdateCheckFromSettings() { StartUpdateCheck(true); }

void RebuildMenu() {
  if (g_menu) DestroyMenu(g_menu);
  g_menu = CreatePopupMenu();
  AppendMenuW(g_menu, MF_STRING | (g_settings.enabled ? MF_CHECKED : 0), ID_ENABLE,
              L"Enable Phostide");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_menu, MF_STRING, ID_SETTINGS, L"Settings...");
  if (HasPendingUpdate()) {
    wchar_t updateLabel[96];
    swprintf_s(updateLabel, L"Update available: %s", g_settings.pendingUpdateTag.c_str());
    AppendMenuW(g_menu, MF_STRING, ID_OPEN_UPDATE, updateLabel);
  }
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_menu, MF_STRING, ID_EXIT, L"Exit");
}

void ShowContextMenu(HWND hwnd) {
  RebuildMenu();
  POINT pt{};
  GetCursorPos(&pt);
  SetForegroundWindow(hwnd);
  TrackPopupMenu(g_menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
  PostMessageW(hwnd, WM_NULL, 0, 0);
}

void AddTrayIcon(HWND hwnd, HINSTANCE hi, bool force = false) {
  // Explorer restarts wipe the notification area; NIM_MODIFY may still succeed
  // against a stale GUID entry, so force a delete+add after TaskbarCreated.
  if (g_trayAdded && !force) {
    if (Shell_NotifyIconW(NIM_MODIFY, &g_nid)) return;
    force = true;
  }
  if (force && g_trayAdded) {
    NOTIFYICONDATAW del = g_nid;
    del.uFlags = NIF_GUID;
    Shell_NotifyIconW(NIM_DELETE, &del);
    g_trayAdded = false;
  }
  if (g_trayAdded) return;

  HICON oldIcon = g_nid.hIcon;
  const bool oldOwned = g_trayIconOwned;

  g_nid = {};
  g_nid.cbSize = sizeof(NOTIFYICONDATAW);
  g_nid.hWnd = hwnd;
  // Prefer NIF_GUID so tray visibility prefs survive updates (path/HWND changes).
  g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP | NIF_GUID;
  g_nid.guidItem = phostide::kTrayIconGuid;
  g_nid.uCallbackMessage = WM_TRAY;
  g_trayIconOwned = false;
  g_nid.hIcon = static_cast<HICON>(LoadImageW(
      hi, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
  if (g_nid.hIcon) {
    g_trayIconOwned = true;
  } else {
    g_nid.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(IDI_APPICON));
  }
  if (!g_nid.hIcon) {
    g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  }
  wcsncpy_s(g_nid.szTip, L"Phostide", _TRUNCATE);
  g_nid.uVersion = NOTIFYICON_VERSION_4;

  if (Shell_NotifyIconW(NIM_ADD, &g_nid)) {
    Shell_NotifyIconW(NIM_SETVERSION, &g_nid);
    g_trayAdded = true;
    if (oldOwned && oldIcon && oldIcon != g_nid.hIcon) {
      DestroyIcon(oldIcon);
    }
  } else if (oldOwned && oldIcon && oldIcon != g_nid.hIcon) {
    // Keep previous icon handle if ADD failed.
    DestroyIcon(g_nid.hIcon);
    g_nid.hIcon = oldIcon;
    g_trayIconOwned = oldOwned;
  }
}

void RemoveTrayIcon() {
  if (!g_trayAdded) return;
  g_nid.uFlags = NIF_GUID;
  g_nid.guidItem = phostide::kTrayIconGuid;
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
  if (g_trayIconOwned && g_nid.hIcon) {
    DestroyIcon(g_nid.hIcon);
  }
  g_nid.hIcon = nullptr;
  g_trayIconOwned = false;
  g_trayAdded = false;
}

void ShutdownHost() {
  static bool once = false;
  if (once) return;
  once = true;

  if (g_hwnd) {
    KillTimer(g_hwnd, ID_ANIM_TIMER);
    WTSUnRegisterSessionNotification(g_hwnd);
  }
  if (phostide::SettingsWindowIsOpen()) {
    DestroyWindow(phostide::SettingsWindowHwnd());
  }
  RemoveTrayIcon();
  // Restore stock taskbar in Explorer, then drop our hook so Setup can replace files.
  phostide::RequestTapRestore();
  Sleep(60);
  phostide::ClearTaskbarAccent();
  phostide::DetachExplorerTap();
}

void StartUpdateCheck(bool manual) {
  if (g_updateCheckRunning) return;
  g_updateCheckRunning = true;
  g_updateStatusText.clear();
  phostide::SettingsWindowRefreshUpdateUi();
  std::thread([manual] {
    auto* payload = new UpdateDonePayload();
    payload->manual = manual;
    payload->result = phostide::CheckForUpdates();
    if (g_hwnd) {
      PostMessageW(g_hwnd, WM_UPDATE_DONE, 0, reinterpret_cast<LPARAM>(payload));
    } else {
      delete payload;
    }
  }).detach();
}

void HandleUpdateDone(UpdateDonePayload* payload) {
  std::unique_ptr<UpdateDonePayload> owned(payload);
  g_updateCheckRunning = false;
  if (!owned) return;

  const auto& r = owned->result;
  g_updateStatusText.clear();
  if (r.ok) {
    g_settings.lastUpdateCheckUtc = phostide::UtcNowUnixSeconds();
    if (r.updateAvailable) {
      g_settings.pendingUpdateTag = r.latestTag;
      g_settings.pendingSetupUrl =
          !r.setupUrl.empty() ? r.setupUrl : r.releasePageUrl;
      phostide::SaveSettings(g_settings);
      if (!owned->manual) {
        ShowBalloon(L"Phostide update",
                    L"A newer version is available. Open Settings from the tray to update.");
      }
    } else {
      g_settings.pendingUpdateTag.clear();
      g_settings.pendingSetupUrl.clear();
      phostide::SaveSettings(g_settings);
    }
  } else if (owned->manual) {
    g_updateStatusText =
        r.error.empty() ? L"Could not check for updates." : r.error;
  }

  phostide::SettingsWindowRefreshUpdateUi();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  if (g_taskbarCreatedMsg && msg == g_taskbarCreatedMsg) {
    // Explorer was restarted (by us or the user) — notification icons are gone,
    // and ExplorerTAP must be reinjected into the new tray process/tree.
    if (g_instance) AddTrayIcon(hwnd, g_instance, /*force=*/true);
    ScheduleReinjectAfterTrayRecreate(hwnd);
    return 0;
  }
  switch (msg) {
    case WM_CREATE:
      AddTrayIcon(hwnd, reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance);
      SyncAnimTimer(hwnd);
      SyncIdleTimer(hwnd);
      return 0;
    case WM_TIMER:
      if (wParam == ID_REINJECT_TIMER) {
        KillTimer(hwnd, ID_REINJECT_TIMER);
        StartInjectWorker();
        return 0;
      }
      if (wParam == ID_IDLE_TIMER) {
        PollOledCareIdle(hwnd);
        return 0;
      }
      if (wParam == ID_ANIM_TIMER && g_settings.enabled && !IsThemePaintPaused()) {
        if (RefreshTapBound()) {
          SyncAnimTimer(hwnd);
          ApplyTheme(true);
        } else {
          // Frame tick: accent only (avoid spamming the TAP pipe).
          ApplyTheme(false);
        }
      }
      return 0;
    case WM_INJECT_DONE:
      g_injectInFlight.store(false);
      // Injection may have bounced Explorer — restore the tray icon first.
      if (g_instance) AddTrayIcon(hwnd, g_instance, /*force=*/true);
      // Injection finished — push theme; poll bound so we can drop accent.
      ApplyTheme(true);
      SyncAnimTimer(hwnd);
      SetTimer(hwnd, ID_ANIM_TIMER, kAnimIntervalMs, nullptr);
      return 0;
    case WM_UPDATE_DONE:
      HandleUpdateDone(reinterpret_cast<UpdateDonePayload*>(lParam));
      return 0;
    case WM_TRAY:
      if (LOWORD(lParam) == NIN_BALLOONUSERCLICK) {
        if (HasPendingUpdate()) {
          phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Updates);
        }
      } else if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == WM_RBUTTONUP ||
                 lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
        ShowContextMenu(hwnd);
      } else if (LOWORD(lParam) == WM_LBUTTONUP || lParam == WM_LBUTTONUP) {
        // Left-click toggles Enable (same as tray menu). Soft-suspend in TAP keeps
        // fill refs so the next click can rebind without a full Explorer rescan.
        g_settings.enabled = !g_settings.enabled;
        phostide::SaveSettings(g_settings);
        if (!g_settings.enabled) {
          g_tapBound = false;
          phostide::ResetTapBoundEvent();
        }
        SyncAnimTimer(hwnd);
        SyncIdleTimer(hwnd);
        RefreshOledCareIdleStates();
        ApplyTheme(true);
        phostide::SettingsWindowRefreshPauseUi();
        ShowBalloon(g_settings.enabled ? L"Phostide" : L"Phostide",
                    g_settings.enabled ? L"Themes on." : L"Themes paused. Click again to resume.");
      }
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == ID_ENABLE) {
        g_settings.enabled = !g_settings.enabled;
        phostide::SaveSettings(g_settings);
        if (!g_settings.enabled) {
          g_tapBound = false;
          phostide::ResetTapBoundEvent();
        }
        SyncAnimTimer(hwnd);
        SyncIdleTimer(hwnd);
        RefreshOledCareIdleStates();
        ApplyTheme(true);
        phostide::SettingsWindowRefreshPauseUi();
        return 0;
      }
      if (id == ID_SETTINGS) {
        phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Normal);
        return 0;
      }
      if (id == ID_OPEN_UPDATE) {
        phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Updates);
        return 0;
      }
      if (id == ID_EXIT) {
        DestroyWindow(hwnd);
        return 0;
      }
      return 0;
    }
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return 0;
    case WM_QUERYENDSESSION:
      // Allow Restart Manager / Inno CloseApplications to shut us down.
      return TRUE;
    case WM_ENDSESSION:
      if (wParam) {
        ShutdownHost();
        PostQuitMessage(0);
      }
      return 0;
    case WM_WTSSESSION_CHANGE:
      if (g_settings.pauseWhenLocked) {
        if (wParam == WTS_SESSION_LOCK) {
          g_sessionLocked = true;
          ClearMonitorCareIdle();
          SyncAnimTimer(hwnd);
          SyncIdleTimer(hwnd);
          phostide::ClearTaskbarAccent();
          // Stop TAP paint loop inside explorer.exe while the desktop is covered.
          ApplyTheme(true);
          phostide::SettingsWindowRefreshPauseUi();
        } else if (wParam == WTS_SESSION_UNLOCK) {
          g_sessionLocked = false;
          SyncAnimTimer(hwnd);
          SyncIdleTimer(hwnd);
          RefreshOledCareIdleStates();
          ApplyTheme(true);
          phostide::SettingsWindowRefreshPauseUi();
        }
      }
      return 0;
    case WM_DESTROY:
      ShutdownHost();
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR cmdLine, int) {
  // Setup / scripts: Phostide.exe /exit → ask the live tray instance to quit.
  if (cmdLine && (wcsstr(cmdLine, L"/exit") || wcsstr(cmdLine, L"--exit"))) {
    return phostide::RequestExistingInstanceExit(20000) ? 0 : 1;
  }

  // Single-instance: Create + Wait. Do NOT exit solely on ERROR_ALREADY_EXISTS —
  // ExplorerTAP watches this mutex with OpenMutex, so the name can still exist
  // briefly after a crash/kill. Only bail when a live tray window exists or Wait
  // cannot acquire (another host still owns it).
  HANDLE mutex = CreateMutexW(nullptr, FALSE, phostide::kMutexName.data());
  if (!mutex) {
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS &&
      FindWindowW(L"PhostideTray", nullptr) != nullptr) {
    CloseHandle(mutex);
    return 0;
  }
  const DWORD mutexWait = WaitForSingleObject(mutex, 3000);
  if (mutexWait == WAIT_TIMEOUT) {
    CloseHandle(mutex);
    if (FindWindowW(L"PhostideTray", nullptr) != nullptr) return 0;
    // Stuck ownership (old TAP bug held the mutex). Continue with a fresh name
    // attempt only if no tray — last resort: fail closed so we don't dual-host.
    return 1;
  }

  g_instance = hi;
  g_taskbarCreatedMsg = RegisterWindowMessageW(L"TaskbarCreated");

  // Keep Start / taskbar / notification prefs tied to one identity across updates.
  SetCurrentProcessExplicitAppUserModelID(phostide::kAppUserModelId.data());

  // Ensure Explorer is alive after a previous crash before we touch the shell.
  if (!FindWindowW(L"Shell_TrayWnd", nullptr)) {
    Sleep(1500);
  }

  g_settings = phostide::LoadSettings();
  // Drop stale pending update if this build is already >= cached tag.
  if (!g_settings.pendingUpdateTag.empty() &&
      !phostide::IsVersionNewer(g_settings.pendingUpdateTag, phostide::kAppVersion)) {
    g_settings.pendingUpdateTag.clear();
    g_settings.pendingSetupUrl.clear();
    phostide::SaveSettings(g_settings);
  }

  phostide::SettingsWindowHost host{};
  host.settings = &g_settings;
  host.updateCheckRunning = &g_updateCheckRunning;
  host.updateStatusText = &g_updateStatusText;
  host.sessionLocked = &g_sessionLocked;
  host.fullscreenPaused = &g_fullscreenPaused;
  host.fullscreenHashes = g_fullscreenHashes;
  host.fullscreenHashCount = &g_fullscreenHashCount;
  host.onApplyAppearance = &OnApplyAppearanceFromSettings;
  host.onRequestUpdateCheck = &OnRequestUpdateCheckFromSettings;
  host.onOpenUpdate = &OpenPendingUpdate;
  phostide::SettingsWindowBind(host);

  // Named manual-reset event can still be signaled from a previous session's TAP
  // bind — reset so we don't skip accent / assume fills are live before reinject.
  phostide::ResetTapBoundEvent();
  g_tapBound = false;
  g_boundEvent = CreateEventW(nullptr, TRUE, FALSE, phostide::kTapBoundEvent.data());
  phostide::ResetTapBoundEvent();

  WNDCLASSEXW wc{sizeof(wc)};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hi;
  wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(IDI_APPICON));
  wc.hIconSm = static_cast<HICON>(LoadImageW(
      hi, MAKEINTRESOURCEW(IDI_APPICON), IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.lpszClassName = L"PhostideTray";
  RegisterClassExW(&wc);

  // Hidden top-level tool window (more reliable for tray than HWND_MESSAGE alone).
  g_hwnd = CreateWindowExW(WS_EX_TOOLWINDOW, wc.lpszClassName, phostide::kAppName.data(),
                           WS_POPUP, 0, 0, 0, 0, nullptr, nullptr, hi, nullptr);
  if (!g_hwnd) {
    return 1;
  }
  ShowWindow(g_hwnd, SW_HIDE);

  // Tray immediately — before any injection work.
  AddTrayIcon(g_hwnd, hi);
  SyncAnimTimer(g_hwnd);
  SyncIdleTimer(g_hwnd);
  RefreshOledCareIdleStates();
  ApplyTheme(true);

  WTSRegisterSessionNotification(g_hwnd, NOTIFY_FOR_THIS_SESSION);

  // Inject on a worker so the UI thread stays responsive.
  StartInjectWorker();

  // Daily (or first-run) update check — silent unless newer.
  if (phostide::IsUpdateCheckDue(g_settings.lastUpdateCheckUtc)) {
    StartUpdateCheck(false);
  }

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    HWND settings = phostide::SettingsWindowHwnd();
    if (settings && IsDialogMessageW(settings, &msg)) {
      continue;
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  ShutdownHost();
  if (mutex) {
    ReleaseMutex(mutex);
    CloseHandle(mutex);
  }
  return 0;
}
