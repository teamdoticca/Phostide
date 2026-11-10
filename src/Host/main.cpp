#include "Injector.h"
#include "SettingsStore.h"
#include "SettingsWindow.h"
#include "UpdateCheck.h"
#include "resource.h"
#include "../Common/PhostideCommon.h"
#include "../Common/TaskbarAccent.h"

#include <shellapi.h>
#include <shobjidl.h>
#include <windows.h>
#include <wtsapi32.h>

#include <chrono>
#include <memory>
#include <string>
#include <thread>

namespace {

constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT WM_INJECT_DONE = WM_APP + 2;
constexpr UINT WM_UPDATE_DONE = WM_APP + 3;
constexpr UINT_PTR ID_ANIM_TIMER = 1;
constexpr UINT ID_ENABLE = 100;
constexpr UINT ID_SETTINGS = 110;
constexpr UINT ID_OPEN_UPDATE = 310;
constexpr UINT ID_EXIT = 399;
constexpr UINT kAnimIntervalMs = 33;  // ~30 fps host-side accent animation

HWND g_hwnd = nullptr;
HINSTANCE g_instance = nullptr;
NOTIFYICONDATAW g_nid{};
phostide::Settings g_settings{};
HMENU g_menu = nullptr;
bool g_trayAdded = false;
bool g_trayIconOwned = false;
bool g_sessionLocked = false;
bool g_tapBound = false;
bool g_clearedAccentForTap = false;
bool g_updateCheckRunning = false;
std::wstring g_updateStatusText;
std::chrono::steady_clock::time_point g_animStart = std::chrono::steady_clock::now();
HANDLE g_boundEvent = nullptr;

struct UpdateDonePayload {
  bool manual = false;
  phostide::UpdateCheckResult result;
};

phostide::ThemeCommand MakeCommand() {
  phostide::ThemeCommand cmd;
  cmd.apiVersion = phostide::kTapApiVersion;
  cmd.theme = static_cast<uint32_t>(g_settings.theme);
  cmd.intensity = static_cast<uint32_t>(g_settings.intensity);
  cmd.enabled = g_settings.enabled ? 1u : 0u;
  return cmd;
}

float AnimSeconds() {
  using namespace std::chrono;
  return duration<float>(steady_clock::now() - g_animStart).count();
}

bool RefreshTapBound() {
  if (g_tapBound) return true;
  if (g_boundEvent && WaitForSingleObject(g_boundEvent, 0) == WAIT_OBJECT_0) {
    g_tapBound = true;
  }
  return g_tapBound;
}

void ApplyTheme(bool pushToTap = true) {
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
  const bool run = g_settings.enabled && !g_sessionLocked && !RefreshTapBound();
  if (run) {
    SetTimer(hwnd, ID_ANIM_TIMER, kAnimIntervalMs, nullptr);
  } else {
    KillTimer(hwnd, ID_ANIM_TIMER);
  }
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
  SyncAnimTimer(g_hwnd);
  ApplyTheme(true);
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

void AddTrayIcon(HWND hwnd, HINSTANCE hi) {
  if (g_trayAdded) return;

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
  switch (msg) {
    case WM_CREATE:
      AddTrayIcon(hwnd, reinterpret_cast<LPCREATESTRUCTW>(lParam)->hInstance);
      SyncAnimTimer(hwnd);
      return 0;
    case WM_TIMER:
      if (wParam == ID_ANIM_TIMER && g_settings.enabled && !g_sessionLocked) {
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
        g_settings.enabled = !g_settings.enabled;
        phostide::SaveSettings(g_settings);
        SyncAnimTimer(hwnd);
        ApplyTheme(true);
      }
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == ID_ENABLE) {
        g_settings.enabled = !g_settings.enabled;
        phostide::SaveSettings(g_settings);
        SyncAnimTimer(hwnd);
        ApplyTheme(true);
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
    case WM_WTSSESSION_CHANGE:
      if (g_settings.pauseWhenLocked) {
        if (wParam == WTS_SESSION_LOCK) {
          g_sessionLocked = true;
          SyncAnimTimer(hwnd);
          phostide::ClearTaskbarAccent();
        } else if (wParam == WTS_SESSION_UNLOCK) {
          g_sessionLocked = false;
          SyncAnimTimer(hwnd);
          ApplyTheme(true);
        }
      }
      return 0;
    case WM_DESTROY:
      KillTimer(hwnd, ID_ANIM_TIMER);
      RemoveTrayIcon();
      phostide::ClearTaskbarAccent();
      PostQuitMessage(0);
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
}

}  // namespace

int WINAPI wWinMain(HINSTANCE hi, HINSTANCE, PWSTR, int) {
  HANDLE mutex = CreateMutexW(nullptr, TRUE, phostide::kMutexName.data());
  if (!mutex || GetLastError() == ERROR_ALREADY_EXISTS) {
    return 0;
  }

  g_instance = hi;

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
  host.onApplyAppearance = &OnApplyAppearanceFromSettings;
  host.onRequestUpdateCheck = &OnRequestUpdateCheckFromSettings;
  host.onOpenUpdate = &OpenPendingUpdate;
  phostide::SettingsWindowBind(host);

  g_boundEvent = CreateEventW(nullptr, TRUE, FALSE, phostide::kTapBoundEvent.data());

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
  ApplyTheme(true);

  WTSRegisterSessionNotification(g_hwnd, NOTIFY_FOR_THIS_SESSION);

  // Inject on a worker so the UI thread stays responsive.
  std::thread([] {
    std::wstring err;
    phostide::InjectExplorerTap(&err);
    if (g_hwnd) {
      PostMessageW(g_hwnd, WM_INJECT_DONE, 0, 0);
    }
  }).detach();

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

  phostide::ClearTaskbarAccent();
  return 0;
}
