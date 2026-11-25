#include "CareNotice.h"
#include "Diagnostics.h"
#include "FullscreenDetect.h"
#include "Injector.h"
#include "SettingsStore.h"
#include "SettingsWindow.h"
#include "Toast.h"
#include "UpdateCheck.h"
#include "UpdateInstall.h"
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

#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Networking.Connectivity.h>

namespace {

constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT WM_INJECT_DONE = WM_APP + 2;
constexpr UINT WM_UPDATE_DONE = WM_APP + 3;
constexpr UINT WM_UPDATE_PROGRESS = WM_APP + 4;  // wParam = percent + 1 (0 = unknown)
constexpr UINT WM_UPDATE_READY = WM_APP + 5;     // lParam = UpdateReadyPayload*
constexpr UINT WM_TOAST_ACTIVATED = WM_APP + 6;  // lParam = std::wstring* argument
constexpr UINT_PTR ID_ANIM_TIMER = 1;
constexpr UINT_PTR ID_REINJECT_TIMER = 2;
constexpr UINT_PTR ID_IDLE_TIMER = 3;
constexpr UINT_PTR ID_RESUME_TIMER = 4;  // checks the "Pause for 1 hour" deadline
constexpr UINT_PTR ID_UPDATE_TIMER = 5;  // automatic update check / pending notice
constexpr UINT ID_ENABLE = 100;
constexpr UINT ID_SETTINGS = 110;
constexpr UINT ID_PAUSE_HOUR = 120;
constexpr UINT ID_PAUSE_RESTART = 121;
constexpr UINT ID_RESUME = 122;
constexpr UINT ID_PAUSE_INFO = 123;
constexpr UINT ID_THEME_BASE = 200;  // +ThemeId; tray Theme submenu (all displays)
constexpr UINT ID_OPEN_UPDATE = 310;
constexpr UINT ID_EXIT = 399;
constexpr UINT kAnimIntervalMs = 33;  // ~30 fps host-side accent animation
constexpr UINT kReinjectDebounceMs = 1000;
constexpr UINT kIdlePollMs = 1000;       // while awake — cheap attendance
constexpr UINT kIdlePollCareMs = 200;    // while any bar is dimmed — snappy wake
constexpr UINT kFullscreenEnterDwellMs = 750;
constexpr UINT kFullscreenLeaveDwellMs = 300;
constexpr ULONGLONG kTimedPauseMs = 60ull * 60ull * 1000ull;
constexpr UINT kResumeCheckMs = 15000;  // GetTickCount64 counts sleep; WM_TIMER may lag

// Tray quick pause. Never persisted: settings.json keeps enabled=true, so a timed
// or "until restart" pause ends with the next Phostide start at the latest.
enum class TempPause { None, Timed, UntilRestart };

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
uint32_t g_careDimmedCount = 0;  // displays really dimming at the last care refresh
bool g_tapBound = false;
bool g_clearedAccentForTap = false;
bool g_updateCheckRunning = false;
bool g_updateInstallRunning = false;
bool g_showWelcomeAfterInject = false;
std::atomic<bool> g_injectInFlight{false};
std::wstring g_updateStatusText;
TempPause g_tempPause = TempPause::None;
ULONGLONG g_tempPauseDeadline = 0;
std::wstring g_pauseText;  // "Paused until 15:42" / "Paused until Phostide restarts"
std::chrono::steady_clock::time_point g_animStart = std::chrono::steady_clock::now();
HANDLE g_boundEvent = nullptr;

struct UpdateDonePayload {
  bool manual = false;
  phostide::UpdateCheckResult result;
};

// Setup download (background pre-download or "Update now"); UI thread only.
bool g_updateDownloadRunning = false;  // DownloadAndVerifySetup worker in flight
std::wstring g_updateReadyTag;         // verified Setup for this tag is on disk
std::wstring g_prefetchTriedTag;       // one background download per version per run

struct UpdateReadyPayload {
  std::wstring tag;  // pending tag the download was started for
  phostide::SetupDownloadResult result;
};

void ApplyTheme(bool pushToTap);
void SyncAnimTimer(HWND hwnd);
// automatic: not a direct answer to a tray click (OLED Care, update, timed resume);
// skipped while Windows reports a fullscreen app, presentation mode or quiet time.
// Returns false when the balloon was not shown (no tray icon, or suppressed).
bool ShowBalloon(const wchar_t* title, const wchar_t* body, bool automatic = false);
// Short status message: a Windows notification (replaces the previous one, expires
// in minutes) on Setup installs, else a tray balloon. Same `automatic` rules.
bool Notify(const wchar_t* title, const wchar_t* body, bool automatic = false);

// Every host save goes through here so a temporary tray pause is never written.
void PersistSettings() {
  if (g_tempPause == TempPause::None) {
    phostide::SaveSettings(g_settings);
    return;
  }
  phostide::Settings copy = g_settings;
  copy.enabled = true;
  phostide::SaveSettings(copy);
}

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
      phostide::CollectFullscreenCoveredMonitors(covered, phostide::kMaxMonitorSlots);
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
    g_careDimmedCount = 0;
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
    g_careDimmedCount = 0;
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
  uint32_t dimmedCount = 0;  // idle displays that really dim (not paused, not off)
  for (uint32_t i = 0; i < g_careMonitorCount; ++i) {
    const bool unattended =
        (now - g_careMonitors[i].lastAttendedTick) >= thresholdMs;
    const bool idle = systemIdle || unattended;
    if (idle != g_careMonitors[i].idle) {
      g_careMonitors[i].idle = idle;
      changed = true;
    }
    const bool fsPaused =
        g_settings.pauseWhenFullscreen && IsFullscreenHash(g_careMonitors[i].deviceHash);
    const bool off = i < monitors.size() &&
                     phostide::IsMonitorDisabled(g_settings, monitors[i].deviceName);
    if (g_careMonitors[i].idle && !fsPaused && !off) ++dimmedCount;
  }
  if (phostide::CareDimNoticeDue(g_careDimmedCount, dimmedCount)) {
    wchar_t body[80];
    swprintf_s(body, L"OLED Care on — %u display(s) dimmed.", dimmedCount);
    Notify(L"Phostide", body, /*automatic=*/true);
  }
  g_careDimmedCount = dimmedCount;
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
  // Snapshot on the UI thread — MakeCommand reads host globals.
  const phostide::ThemeCommand reapply = MakeCommand();
  std::thread([reapply] {
    std::wstring err;
    phostide::InjectExplorerTap(&err, &reapply);
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

// %TEMP%\Phostide\host.log: host lifetime, notifications and their clicks.
void HostLog(const std::wstring& line) {
  wchar_t temp[MAX_PATH]{};
  if (!GetTempPathW(MAX_PATH, temp)) return;
  const std::wstring dir = std::wstring(temp) + L"Phostide";
  CreateDirectoryW(dir.c_str(), nullptr);
  HANDLE f = CreateFileW((dir + L"\\host.log").c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
                         nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return;
  SYSTEMTIME st{};
  GetLocalTime(&st);
  wchar_t prefix[64];
  swprintf_s(prefix, L"%04u-%02u-%02u %02u:%02u:%02u ", st.wYear, st.wMonth, st.wDay, st.wHour,
             st.wMinute, st.wSecond);
  const std::wstring out = prefix + line + L"\r\n";
  DWORD written = 0;
  WriteFile(f, out.c_str(), static_cast<DWORD>(out.size() * sizeof(wchar_t)), &written, nullptr);
  CloseHandle(f);
}

// An abrupt exit otherwise leaves no trace (no WER report on many machines).
LONG WINAPI LogUnhandledException(EXCEPTION_POINTERS* info) {
  wchar_t text[160];
  swprintf_s(text, L"crash: exception 0x%08lX at %p",
             info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionCode : 0ul,
             info && info->ExceptionRecord ? info->ExceptionRecord->ExceptionAddress : nullptr);
  HostLog(text);
  return EXCEPTION_CONTINUE_SEARCH;
}

bool ShowBalloon(const wchar_t* title, const wchar_t* body, bool automatic) {
  if (!g_trayAdded) return false;
  if (automatic) {
    QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
    if (SUCCEEDED(SHQueryUserNotificationState(&state)) &&
        !phostide::AutomaticBalloonAllowed(state)) {
      return false;
    }
  }
  NOTIFYICONDATAW tip = g_nid;
  tip.uFlags = NIF_INFO | NIF_GUID;
  tip.guidItem = phostide::kTrayIconGuid;
  tip.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
  wcsncpy_s(tip.szInfoTitle, title, _TRUNCATE);
  wcsncpy_s(tip.szInfo, body, _TRUNCATE);
  return Shell_NotifyIconW(NIM_MODIFY, &tip) != FALSE;
}

bool NotificationsWelcome() {
  QUERY_USER_NOTIFICATION_STATE state = QUNS_ACCEPTS_NOTIFICATIONS;
  return FAILED(SHQueryUserNotificationState(&state)) ||
         phostide::AutomaticBalloonAllowed(state);
}

bool Notify(const wchar_t* title, const wchar_t* body, bool automatic) {
  if (!phostide::ToastsAvailable()) return ShowBalloon(title, body, automatic);
  if (automatic && !NotificationsWelcome()) return false;
  phostide::ToastContent t;
  t.title = title;
  t.body = body;
  t.launchArgument = phostide::kToastArgOpenSettings;
  t.tag = L"status";       // each status message replaces the previous one
  t.expiryMinutes = 10;    // and does not pile up in Notification Center
  const bool toast = phostide::ShowToast(t);
  HostLog(std::wstring(L"notify ") + (toast ? L"toast: " : L"toast failed: ") + title);
  return toast || ShowBalloon(title, body, automatic);
}

void OpenReleasePageInBrowser() {
  const wchar_t* url = L"https://github.com/teamdoticca/Phostide/releases/latest";
  if (!g_settings.pendingSetupUrl.empty() &&
      !phostide::IsTrustedSetupUrl(g_settings.pendingSetupUrl)) {
    url = g_settings.pendingSetupUrl.c_str();  // release page from the API
  }
  ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

void ReportUpdateProgress(void*, int percent) {
  if (g_hwnd) PostMessageW(g_hwnd, WM_UPDATE_PROGRESS, static_cast<WPARAM>(percent + 1), 0);
}

// Settings › System › "Update now": download the signed Setup, verify it, run it
// silently and exit. Setup restarts Explorer if the TAP changed and relaunches us.
bool RunningFromScoop() {
  static const bool scoop = [] {
    wchar_t exe[MAX_PATH]{};
    const DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH);
    return n > 0 && n < MAX_PATH && phostide::IsScoopInstallPath(exe);
  }();
  return scoop;
}

// The pending update can be installed in-app (signed Setup from our releases).
bool CanInstallInApp() {
  return HasPendingUpdate() && !RunningFromScoop() &&
         phostide::IsTrustedSetupUrl(g_settings.pendingSetupUrl);
}

// Metered / roaming / near the data cap: no unasked background download.
bool OnMeteredConnection() {
  namespace wnc = winrt::Windows::Networking::Connectivity;
  try {
    const auto profile = wnc::NetworkInformation::GetInternetConnectionProfile();
    if (!profile) return false;  // offline: the download simply fails
    const auto cost = profile.GetConnectionCost();
    return cost.Roaming() || cost.OverDataLimit() || cost.ApproachingDataLimit() ||
           cost.NetworkCostType() == wnc::NetworkCostType::Fixed ||
           cost.NetworkCostType() == wnc::NetworkCostType::Variable;
  } catch (...) {
    return false;
  }
}

// Download + verify the pending Setup (reuses one already on disk). The result
// arrives as WM_UPDATE_READY; a running download is never started twice.
void StartUpdateDownload() {
  if (g_updateDownloadRunning) return;
  g_updateDownloadRunning = true;
  const std::wstring url = g_settings.pendingSetupUrl;
  const std::wstring tag = g_settings.pendingUpdateTag;
  std::thread([url, tag] {
    auto* payload = new UpdateReadyPayload{
        tag, phostide::DownloadAndVerifySetup(url, &ReportUpdateProgress, nullptr)};
    if (g_hwnd) {
      PostMessageW(g_hwnd, WM_UPDATE_READY, 0, reinterpret_cast<LPARAM>(payload));
    } else {
      delete payload;
    }
  }).detach();
}

// Background pre-download right after a check finds a version, so Install update
// starts Setup at once. True while it runs (the announcement waits for it).
bool PrefetchPendingUpdate() {
  if (g_updateDownloadRunning) return true;
  const std::wstring& tag = g_settings.pendingUpdateTag;
  if (g_updateReadyTag == tag || g_prefetchTriedTag == tag) return false;
  if (!CanInstallInApp() || OnMeteredConnection()) return false;
  g_prefetchTriedTag = tag;
  HostLog(L"update " + tag + L": downloading in the background");
  StartUpdateDownload();
  return true;
}

void StartUpdateInstall() {
  if (g_updateInstallRunning) return;
  // Scoop owns this copy: Setup would install a second one under Programs.
  if (RunningFromScoop()) {
    g_updateStatusText = L"Installed with Scoop - update with: scoop update phostide";
    phostide::SettingsWindowRefreshUpdateUi();
    return;
  }
  if (!CanInstallInApp()) {
    OpenReleasePageInBrowser();
    return;
  }
  g_updateInstallRunning = true;
  g_updateStatusText = g_updateReadyTag == g_settings.pendingUpdateTag
                           ? L"Starting the installer..."
                           : L"Downloading " + g_settings.pendingUpdateTag + L"...";
  phostide::SettingsWindowRefreshUpdateUi();
  // Reuses the verified pre-download; while that still runs, its result installs.
  StartUpdateDownload();
}

void HandleUpdateProgress(int percent) {
  if (!g_updateInstallRunning) return;
  wchar_t text[128];
  if (percent >= 0) {
    swprintf_s(text, L"Downloading %s... %d%%", g_settings.pendingUpdateTag.c_str(), percent);
  } else {
    swprintf_s(text, L"Downloading %s...", g_settings.pendingUpdateTag.c_str());
  }
  g_updateStatusText = text;
  phostide::SettingsWindowRefreshUpdateUi();
}

void TryNotifyPendingUpdate();

void HandleUpdateReady(HWND hwnd, UpdateReadyPayload* raw) {
  std::unique_ptr<UpdateReadyPayload> payload(raw);
  g_updateDownloadRunning = false;
  if (!payload) return;
  const auto& r = payload->result;
  // A newer check may have replaced the pending version meanwhile.
  const bool current = HasPendingUpdate() && payload->tag == g_settings.pendingUpdateTag;
  HostLog(L"update " + payload->tag +
          (r.ok ? (r.fromCache ? L": verified Setup already downloaded"
                               : L": downloaded and verified")
                : L": download failed: " + r.error));
  if (r.ok && current) g_updateReadyTag = payload->tag;

  if (!g_updateInstallRunning) {
    // Background pre-download: announce now (also after a failure; Install update
    // then downloads again).
    TryNotifyPendingUpdate();
    phostide::SettingsWindowRefreshUpdateUi();
    return;
  }
  if (r.ok && !current) {
    StartUpdateDownload();  // install what is pending now
    return;
  }
  std::wstring error;
  if (r.ok) {
    g_updateStatusText = L"Installing - Phostide will close and start again.";
    phostide::SettingsWindowRefreshUpdateUi();
    if (phostide::LaunchSetupForUpdate(r.setupPath, &error)) {
      // Exit now (restores stock brushes, drops the hook) so Setup can replace files.
      DestroyWindow(hwnd);
      return;
    }
  } else {
    error = r.error;
  }
  g_updateInstallRunning = false;
  g_updateStatusText = (error.empty() ? L"Update failed." : error) +
                       L" Opened the release page instead.";
  phostide::SettingsWindowRefreshUpdateUi();
  OpenReleasePageInBrowser();
}

void OpenPendingUpdate() { StartUpdateInstall(); }

// Announce a found update once per version.
// Windows notification (Setup installs): Install update / Later buttons; while a
// fullscreen app or Do Not Disturb is on it goes silently to Notification Center.
// Tray balloon (portable / Scoop): held back then, retried on the next tick.
void TryNotifyPendingUpdate() {
  if (!HasPendingUpdate() || g_updateInstallRunning) return;
  if (!phostide::UpdateNoticeDue(g_settings.pendingUpdateTag, g_settings.notifiedUpdateTag)) {
    return;
  }
  // Download first, so the announcement's Install update starts Setup at once;
  // HandleUpdateReady comes back here when it is done.
  if (PrefetchPendingUpdate()) return;
  const bool ready = g_updateReadyTag == g_settings.pendingUpdateTag;
  bool shown = false;
  if (phostide::ToastsAvailable()) {
    phostide::ToastContent t;
    t.title = L"Phostide " + g_settings.pendingUpdateTag + L" is available";
    t.body = RunningFromScoop() ? L"Update with: scoop update phostide"
             : ready ? L"Downloaded and ready: Phostide closes, updates and starts again."
                     : L"Install now: Phostide closes, updates and starts again.";
    t.launchArgument = phostide::kToastArgOpenUpdates;
    if (!RunningFromScoop()) {
      t.buttons = {{L"Install update", phostide::kToastArgInstallUpdate},
                   {L"Later", phostide::kToastArgDismiss}};
    }
    t.tag = L"update";
    t.silentToCenter = !NotificationsWelcome();
    shown = phostide::ShowToast(t);
    HostLog(L"update toast " + g_settings.pendingUpdateTag + (shown ? L" shown" : L" failed") +
            (t.silentToCenter ? L" (to Notification Center only)" : L""));
  }
  if (!shown) {
    const std::wstring body = L"Phostide " + g_settings.pendingUpdateTag +
                              L" is available. Right-click the tray icon > Install update.";
    shown = ShowBalloon(L"Phostide update", body.c_str(), /*automatic=*/true);
  }
  if (shown) {
    g_settings.notifiedUpdateTag = g_settings.pendingUpdateTag;
    PersistSettings();
  }
}

// Tray "Install update" and the notification's button.
void InstallUpdateFromShell(HWND hwnd) {
  if (RunningFromScoop()) {
    // Scoop owns updates: Settings shows the "scoop update phostide" hint.
    phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Updates);
  }
  // Same verified path as Settings > Update now (download, signature, silent
  // Setup, relaunch); progress shows in Settings if it is open.
  StartUpdateInstall();
  // Already downloaded: Setup starts within a second, nothing to report.
  if (g_updateInstallRunning && g_updateReadyTag != g_settings.pendingUpdateTag) {
    const std::wstring body = L"Downloading " + g_settings.pendingUpdateTag +
                              L". Phostide restarts when it is installed.";
    Notify(L"Phostide update", body.c_str());
  }
}

void HandleToastActivated(HWND hwnd, const std::wstring& argument) {
  HostLog(L"notification clicked: " + (argument.empty() ? std::wstring(L"(body)") : argument));
  if (argument == phostide::kToastArgInstallUpdate) {
    InstallUpdateFromShell(hwnd);
  } else if (argument == phostide::kToastArgOpenUpdates) {
    phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Updates);
  } else if (argument == phostide::kToastArgOpenSettings) {
    phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Normal);
  }
  // kToastArgDismiss ("Later"): nothing; the notification is gone already.
}

void StartUpdateCheck(bool manual);

// Automatic updates: first tick kAutoUpdateFirstDelayMs after startup, then hourly.
// Network only when a day has passed since the last check (IsUpdateCheckDue).
void OnUpdateTimer(HWND hwnd) {
  if (g_injectInFlight.load()) {
    // Taskbar hook still running (it can include an Explorer restart): come back.
    SetTimer(hwnd, ID_UPDATE_TIMER, phostide::kAutoUpdateRetryMs, nullptr);
    return;
  }
  SetTimer(hwnd, ID_UPDATE_TIMER, phostide::kAutoUpdateTickMs, nullptr);
  if (!g_settings.autoCheckUpdates) return;
  TryNotifyPendingUpdate();
  if (phostide::IsUpdateCheckDue(g_settings.lastUpdateCheckUtc)) {
    StartUpdateCheck(false);
  }
}

void OnApplyAppearanceFromSettings() {
  g_animStart = std::chrono::steady_clock::now();
  PersistSettings();
  SyncIdleTimer(g_hwnd);
  RefreshOledCareIdleStates();
  RefreshFullscreenPauseState();
  SyncAnimTimer(g_hwnd);
  ApplyTheme(true);
  phostide::SettingsWindowRefreshPauseUi();
}

void StartUpdateCheck(bool manual);

void OnRequestUpdateCheckFromSettings() { StartUpdateCheck(true); }

std::wstring TrayTipText() {
  if (g_tempPause != TempPause::None) return L"Phostide - " + g_pauseText;
  return g_settings.enabled ? L"Phostide" : L"Phostide - paused";
}

void UpdateTrayTip() {
  if (!g_trayAdded) return;
  wcsncpy_s(g_nid.szTip, TrayTipText().c_str(), _TRUNCATE);
  NOTIFYICONDATAW tip = g_nid;
  tip.uFlags = NIF_TIP | NIF_SHOWTIP | NIF_GUID;
  tip.guidItem = phostide::kTrayIconGuid;
  Shell_NotifyIconW(NIM_MODIFY, &tip);
}

// Everything that has to follow a change of g_settings.enabled.
void ApplyEnabledChange(HWND hwnd) {
  PersistSettings();
  if (!g_settings.enabled) {
    g_tapBound = false;
    phostide::ResetTapBoundEvent();
  }
  SyncAnimTimer(hwnd);
  SyncIdleTimer(hwnd);
  RefreshOledCareIdleStates();
  ApplyTheme(true);
  UpdateTrayTip();
  phostide::SettingsWindowRefreshPauseUi();
}

void ClearTempPause(HWND hwnd) {
  g_tempPause = TempPause::None;
  g_tempPauseDeadline = 0;
  g_pauseText.clear();
  KillTimer(hwnd, ID_RESUME_TIMER);
}

void SetPaintEnabled(HWND hwnd, bool on) {
  ClearTempPause(hwnd);
  g_settings.enabled = on;
  ApplyEnabledChange(hwnd);
}

std::wstring LocalTimeIn(ULONGLONG fromNowMs) {
  FILETIME ft{};
  GetSystemTimeAsFileTime(&ft);
  ULARGE_INTEGER t{};
  t.LowPart = ft.dwLowDateTime;
  t.HighPart = ft.dwHighDateTime;
  t.QuadPart += fromNowMs * 10000ull;
  ft.dwLowDateTime = t.LowPart;
  ft.dwHighDateTime = t.HighPart;
  SYSTEMTIME utc{};
  SYSTEMTIME local{};
  FileTimeToSystemTime(&ft, &utc);
  SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local);
  wchar_t buf[64]{};
  if (!GetTimeFormatEx(LOCALE_NAME_USER_DEFAULT, TIME_NOSECONDS, &local, nullptr, buf, 64)) {
    swprintf_s(buf, L"%02u:%02u", local.wHour, local.wMinute);
  }
  return buf;
}

void StartTempPause(HWND hwnd, TempPause kind) {
  ClearTempPause(hwnd);
  g_tempPause = kind;
  if (kind == TempPause::Timed) {
    g_tempPauseDeadline = GetTickCount64() + kTimedPauseMs;
    g_pauseText = L"Paused until " + LocalTimeIn(kTimedPauseMs);
    SetTimer(hwnd, ID_RESUME_TIMER, kResumeCheckMs, nullptr);
  } else {
    g_pauseText = L"Paused until Phostide restarts";
  }
  g_settings.enabled = false;
  ApplyEnabledChange(hwnd);
  Notify(L"Phostide", (g_pauseText + L". Click the tray icon to resume now.").c_str());
}

void CheckTimedPause(HWND hwnd) {
  if (g_tempPause != TempPause::Timed) {
    KillTimer(hwnd, ID_RESUME_TIMER);
    return;
  }
  if (GetTickCount64() < g_tempPauseDeadline) return;
  SetPaintEnabled(hwnd, true);
  Notify(L"Phostide", L"Themes on again.", /*automatic=*/true);
}

// Tray Theme menu: like Taskbar tab "Apply to All", but each display keeps its
// own intensity.
void SetThemeForAllDisplays(phostide::ThemeId theme) {
  g_settings.theme = theme;
  for (const auto& mon : phostide::EnumerateAttachedMonitors()) {
    phostide::MonitorAppearance a =
        phostide::ResolveMonitorAppearance(g_settings, mon.deviceName);
    a.theme = theme;
    phostide::UpsertMonitorAppearance(g_settings, mon.deviceName, a);
  }
  OnApplyAppearanceFromSettings();
  phostide::SettingsWindowReloadAppearance();
}

// Theme to check in the tray menu, or Count when displays use different themes.
phostide::ThemeId CommonDisplayTheme() {
  const auto monitors = phostide::EnumerateAttachedMonitors();
  if (monitors.empty()) return g_settings.theme;
  phostide::ThemeId common = phostide::ThemeId::Count;
  for (const auto& mon : monitors) {
    const auto t = phostide::ResolveMonitorAppearance(g_settings, mon.deviceName).theme;
    if (common == phostide::ThemeId::Count) {
      common = t;
    } else if (common != t) {
      return phostide::ThemeId::Count;
    }
  }
  return common;
}

bool CopyDiagnosticsFromSettings(HWND owner) {
  phostide::DiagnosticsHostState state;
  state.pauseText = g_pauseText;
  state.sessionLocked = g_sessionLocked;
  state.fullscreenPaused = g_fullscreenPaused;
  state.tapBound = g_tapBound;
  return phostide::CopyTextToClipboard(owner,
                                       phostide::BuildDiagnosticsReport(g_settings, state));
}

void RebuildMenu() {
  if (g_menu) DestroyMenu(g_menu);
  g_menu = CreatePopupMenu();
  if (g_tempPause != TempPause::None) {
    AppendMenuW(g_menu, MF_STRING | MF_GRAYED, ID_PAUSE_INFO, g_pauseText.c_str());
    AppendMenuW(g_menu, MF_STRING, ID_RESUME, L"Resume now");
  } else {
    AppendMenuW(g_menu, MF_STRING | (g_settings.enabled ? MF_CHECKED : 0), ID_ENABLE,
                L"Enable Phostide");
    if (g_settings.enabled) {
      AppendMenuW(g_menu, MF_STRING, ID_PAUSE_HOUR, L"Pause for 1 hour");
      AppendMenuW(g_menu, MF_STRING, ID_PAUSE_RESTART, L"Pause until restart");
    }
  }
  HMENU themes = CreatePopupMenu();
  for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
    AppendMenuW(themes, MF_STRING, ID_THEME_BASE + i,
                phostide::ThemeName(phostide::ThemeFromIndex(i)));
  }
  const phostide::ThemeId current = CommonDisplayTheme();
  if (current != phostide::ThemeId::Count) {
    CheckMenuRadioItem(themes, ID_THEME_BASE,
                       ID_THEME_BASE + static_cast<UINT>(phostide::ThemeId::Count) - 1,
                       ID_THEME_BASE + static_cast<UINT>(current), MF_BYCOMMAND);
  }
  AppendMenuW(g_menu, MF_POPUP, reinterpret_cast<UINT_PTR>(themes), L"Theme (all displays)");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_menu, MF_STRING, ID_SETTINGS, L"Settings...");
  if (g_updateInstallRunning) {
    AppendMenuW(g_menu, MF_STRING | MF_GRAYED, ID_OPEN_UPDATE, L"Installing update...");
  } else if (HasPendingUpdate()) {
    wchar_t updateLabel[96];
    swprintf_s(updateLabel, L"Install update %s", g_settings.pendingUpdateTag.c_str());
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
  wcsncpy_s(g_nid.szTip, TrayTipText().c_str(), _TRUNCATE);
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
  HostLog(L"exit");

  if (g_hwnd) {
    KillTimer(g_hwnd, ID_ANIM_TIMER);
    WTSUnRegisterSessionNotification(g_hwnd);
  }
  if (phostide::SettingsWindowIsOpen()) {
    DestroyWindow(phostide::SettingsWindowHwnd());
  }
  RemoveTrayIcon();
  // Nothing would answer their buttons once we are gone.
  phostide::ToastShutdown();
  // Restore stock taskbar in Explorer, then drop our hook so Setup can replace files.
  phostide::RequestTapRestore();
  Sleep(60);
  phostide::ClearTaskbarAccent();
  phostide::DetachExplorerTap();
  // Exit during a shell bounce must not leave the user without Explorer.
  phostide::FinishPendingExplorerRestart();
}

void StartUpdateCheck(bool manual) {
  if (g_updateCheckRunning) return;
  g_updateCheckRunning = true;
  if (!g_updateInstallRunning) g_updateStatusText.clear();
  phostide::SettingsWindowRefreshUpdateUi();
  const bool includePrereleases = g_settings.updateIncludePrereleases;
  std::thread([manual, includePrereleases] {
    auto* payload = new UpdateDonePayload();
    payload->manual = manual;
    payload->result = phostide::CheckForUpdates(includePrereleases);
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
  if (!g_updateInstallRunning) g_updateStatusText.clear();
  if (r.ok) {
    g_settings.lastUpdateCheckUtc = phostide::UtcNowUnixSeconds();
    if (r.updateAvailable) {
      g_settings.pendingUpdateTag = r.latestTag;
      g_settings.pendingSetupUrl =
          !r.setupUrl.empty() ? r.setupUrl : r.releasePageUrl;
      if (owned->manual) {
        // Seen in Settings already: no notification for this version later, but
        // still downloaded ahead so Update now starts Setup at once.
        g_settings.notifiedUpdateTag = r.latestTag;
      }
      PersistSettings();
      if (owned->manual) {
        PrefetchPendingUpdate();
      } else {
        TryNotifyPendingUpdate();
      }
    } else {
      g_settings.pendingUpdateTag.clear();
      g_settings.pendingSetupUrl.clear();
      PersistSettings();
      if (!g_updateDownloadRunning) {
        g_updateReadyTag.clear();
        phostide::ClearUpdateDownloads();
      }
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
      if (wParam == ID_RESUME_TIMER) {
        CheckTimedPause(hwnd);
        return 0;
      }
      if (wParam == ID_UPDATE_TIMER) {
        OnUpdateTimer(hwnd);
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
      // First run: open Welcome once the taskbar is already painting, so the
      // user sees the effect while choosing which displays are OLED.
      if (g_showWelcomeAfterInject) {
        g_showWelcomeAfterInject = false;
        phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Welcome);
      }
      return 0;
    case WM_UPDATE_DONE:
      HandleUpdateDone(reinterpret_cast<UpdateDonePayload*>(lParam));
      return 0;
    case WM_UPDATE_PROGRESS:
      HandleUpdateProgress(static_cast<int>(wParam) - 1);
      return 0;
    case WM_TOAST_ACTIVATED: {
      std::unique_ptr<std::wstring> argument(reinterpret_cast<std::wstring*>(lParam));
      if (argument) HandleToastActivated(hwnd, *argument);
      return 0;
    }
    case WM_UPDATE_READY:
      HandleUpdateReady(hwnd, reinterpret_cast<UpdateReadyPayload*>(lParam));
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
        // During a timed / until-restart pause this resumes.
        SetPaintEnabled(hwnd, !g_settings.enabled);
        Notify(L"Phostide",
                    g_settings.enabled ? L"Themes on." : L"Themes paused. Click again to resume.");
      }
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == ID_ENABLE) {
        SetPaintEnabled(hwnd, !g_settings.enabled);
        return 0;
      }
      if (id == ID_RESUME) {
        SetPaintEnabled(hwnd, true);
        return 0;
      }
      if (id == ID_PAUSE_HOUR) {
        StartTempPause(hwnd, TempPause::Timed);
        return 0;
      }
      if (id == ID_PAUSE_RESTART) {
        StartTempPause(hwnd, TempPause::UntilRestart);
        return 0;
      }
      if (id >= static_cast<int>(ID_THEME_BASE) &&
          id < static_cast<int>(ID_THEME_BASE) + static_cast<int>(phostide::ThemeId::Count)) {
        SetThemeForAllDisplays(phostide::ThemeFromIndex(id - static_cast<int>(ID_THEME_BASE)));
        return 0;
      }
      if (id == ID_SETTINGS) {
        phostide::SettingsWindowShow(hwnd, g_instance, phostide::SettingsFocus::Normal);
        return 0;
      }
      if (id == ID_OPEN_UPDATE) {
        InstallUpdateFromShell(hwnd);
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
  SetUnhandledExceptionFilter(&LogUnhandledException);
  HostLog(std::wstring(L"start ") + phostide::kAppVersion.data() +
          (phostide::ToastsAvailable() ? L" (Windows notifications)" : L" (tray balloons)"));

  // Keep Start / taskbar / notification prefs tied to one identity across updates.
  SetCurrentProcessExplicitAppUserModelID(phostide::kAppUserModelId.data());
  // UI thread is an STA: Windows notifications (WinRT) are created here.
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

  // Ensure Explorer is alive after a previous crash before we touch the shell.
  if (!FindWindowW(L"Shell_TrayWnd", nullptr)) {
    Sleep(1500);
  }

  // First run (no settings file yet): persist the OLED-first defaults right away so
  // the Welcome shows once even if it is dismissed, then open it after inject.
  g_showWelcomeAfterInject = !phostide::SettingsFileExists();
  g_settings = phostide::LoadSettings();
  if (g_showWelcomeAfterInject) PersistSettings();
  // Drop stale pending update if this build is already >= cached tag.
  if (!g_settings.pendingUpdateTag.empty() &&
      !phostide::IsVersionNewer(g_settings.pendingUpdateTag, phostide::kAppVersion)) {
    g_settings.pendingUpdateTag.clear();
    g_settings.pendingSetupUrl.clear();
    PersistSettings();
  }
  // Nothing pending (e.g. just updated): drop the downloaded Setup.
  if (g_settings.pendingUpdateTag.empty()) phostide::ClearUpdateDownloads();

  phostide::SettingsWindowHost host{};
  host.updateReadyTag = &g_updateReadyTag;
  host.settings = &g_settings;
  host.updateCheckRunning = &g_updateCheckRunning;
  host.updateInstallRunning = &g_updateInstallRunning;
  host.updateStatusText = &g_updateStatusText;
  host.sessionLocked = &g_sessionLocked;
  host.fullscreenPaused = &g_fullscreenPaused;
  host.fullscreenHashes = g_fullscreenHashes;
  host.fullscreenHashCount = &g_fullscreenHashCount;
  host.onApplyAppearance = &OnApplyAppearanceFromSettings;
  host.onRequestUpdateCheck = &OnRequestUpdateCheckFromSettings;
  host.onOpenUpdate = &OpenPendingUpdate;
  host.pauseText = &g_pauseText;
  host.onCopyDiagnostics = &CopyDiagnosticsFromSettings;
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
  phostide::ToastInit(g_hwnd, WM_TOAST_ACTIVATED);

  // Tray immediately — before any injection work.
  AddTrayIcon(g_hwnd, hi);
  SyncAnimTimer(g_hwnd);
  SyncIdleTimer(g_hwnd);
  RefreshOledCareIdleStates();
  ApplyTheme(true);

  WTSRegisterSessionNotification(g_hwnd, NOTIFY_FOR_THIS_SESSION);

  // Inject on a worker so the UI thread stays responsive.
  StartInjectWorker();

  // Automatic update check: never at startup (it would compete with the taskbar
  // hook); first tick a couple of minutes in, then hourly (network once a day).
  SetTimer(g_hwnd, ID_UPDATE_TIMER, phostide::kAutoUpdateFirstDelayMs, nullptr);

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
