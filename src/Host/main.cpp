#include "Injector.h"
#include "SettingsStore.h"
#include "resource.h"
#include "../Common/PhostideCommon.h"
#include "../Common/TaskbarAccent.h"

#include <shellapi.h>
#include <windows.h>
#include <wtsapi32.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>

namespace {

constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT WM_INJECT_DONE = WM_APP + 2;
constexpr UINT_PTR ID_ANIM_TIMER = 1;
constexpr UINT ID_TRAY = 1;
constexpr UINT ID_ENABLE = 100;
constexpr UINT ID_THEME_BASE = 200;
constexpr UINT ID_INTENSITY_DEC = 300;
constexpr UINT ID_INTENSITY_INC = 301;
constexpr UINT ID_AUTOSTART = 302;
constexpr UINT ID_EXIT = 399;
constexpr UINT kAnimIntervalMs = 33;  // ~30 fps host-side accent animation

HWND g_hwnd = nullptr;
NOTIFYICONDATAW g_nid{};
phostide::Settings g_settings{};
HMENU g_menu = nullptr;
bool g_trayAdded = false;
bool g_trayIconOwned = false;
bool g_sessionLocked = false;
bool g_tapBound = false;
bool g_clearedAccentForTap = false;
std::chrono::steady_clock::time_point g_animStart = std::chrono::steady_clock::now();
HANDLE g_boundEvent = nullptr;

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

void RebuildMenu() {
  if (g_menu) DestroyMenu(g_menu);
  g_menu = CreatePopupMenu();
  AppendMenuW(g_menu, MF_STRING | (g_settings.enabled ? MF_CHECKED : 0), ID_ENABLE,
              L"Enable Phostide");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
    const auto id = phostide::ThemeFromIndex(i);
    AppendMenuW(g_menu, MF_STRING | (g_settings.theme == id ? MF_CHECKED : 0),
                ID_THEME_BASE + i, phostide::ThemeName(id));
  }
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  wchar_t intensityLabel[64];
  swprintf_s(intensityLabel, L"Intensity: %d%%", g_settings.intensity);
  AppendMenuW(g_menu, MF_STRING | MF_GRAYED, 0, intensityLabel);
  AppendMenuW(g_menu, MF_STRING, ID_INTENSITY_DEC, L"Intensity -");
  AppendMenuW(g_menu, MF_STRING, ID_INTENSITY_INC, L"Intensity +");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_menu, MF_STRING | (g_settings.startWithWindows ? MF_CHECKED : 0), ID_AUTOSTART,
              L"Start with Windows");
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
  g_nid.uID = ID_TRAY;
  g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;
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
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
  if (g_trayIconOwned && g_nid.hIcon) {
    DestroyIcon(g_nid.hIcon);
  }
  g_nid.hIcon = nullptr;
  g_trayIconOwned = false;
  g_trayAdded = false;
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
    case WM_TRAY:
      if (LOWORD(lParam) == WM_CONTEXTMENU || LOWORD(lParam) == WM_RBUTTONUP ||
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
      } else if (id >= ID_THEME_BASE &&
                 id < ID_THEME_BASE + static_cast<int>(phostide::ThemeId::Count)) {
        g_settings.theme = phostide::ThemeFromIndex(id - ID_THEME_BASE);
        g_animStart = std::chrono::steady_clock::now();
      } else if (id == ID_INTENSITY_DEC) {
        g_settings.intensity = (std::max)(0, g_settings.intensity - 5);
      } else if (id == ID_INTENSITY_INC) {
        g_settings.intensity = (std::min)(100, g_settings.intensity + 5);
      } else if (id == ID_AUTOSTART) {
        g_settings.startWithWindows = !g_settings.startWithWindows;
      } else if (id == ID_EXIT) {
        DestroyWindow(hwnd);
        return 0;
      }
      phostide::SaveSettings(g_settings);
      SyncAnimTimer(hwnd);
      ApplyTheme(true);
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

  // Ensure Explorer is alive after a previous crash before we touch the shell.
  if (!FindWindowW(L"Shell_TrayWnd", nullptr)) {
    Sleep(1500);
  }

  g_settings = phostide::LoadSettings();
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

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  phostide::ClearTaskbarAccent();
  return 0;
}
