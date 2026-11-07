#include "Injector.h"
#include "SettingsStore.h"
#include "../Common/PhostideCommon.h"

#include <shellapi.h>
#include <windows.h>
#include <wtsapi32.h>

#include <algorithm>
#include <string>

namespace {

constexpr UINT WM_TRAY = WM_APP + 1;
constexpr UINT ID_TRAY = 1;
constexpr UINT ID_ENABLE = 100;
constexpr UINT ID_THEME_BASE = 200;
constexpr UINT ID_INTENSITY_DEC = 300;
constexpr UINT ID_INTENSITY_INC = 301;
constexpr UINT ID_AUTOSTART = 302;
constexpr UINT ID_EXIT = 399;

HWND g_hwnd = nullptr;
NOTIFYICONDATAW g_nid{};
phostide::Settings g_settings{};
HMENU g_menu = nullptr;

void PushThemeToTap() {
  phostide::ThemeCommand cmd;
  cmd.apiVersion = phostide::kTapApiVersion;
  cmd.theme = static_cast<uint32_t>(g_settings.theme);
  cmd.intensity = static_cast<uint32_t>(g_settings.intensity);
  cmd.enabled = g_settings.enabled ? 1u : 0u;
  phostide::SendThemeCommand(cmd);
}

void RebuildMenu() {
  if (g_menu) DestroyMenu(g_menu);
  g_menu = CreatePopupMenu();
  AppendMenuW(g_menu, MF_STRING | (g_settings.enabled ? MF_CHECKED : 0), ID_ENABLE,
              L"Enable Phostide");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
    const auto id = phostide::ThemeFromIndex(i);
    AppendMenuW(g_menu,
                MF_STRING | (g_settings.theme == id ? MF_CHECKED : 0),
                ID_THEME_BASE + i, phostide::ThemeName(id));
  }
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  wchar_t intensityLabel[64];
  swprintf_s(intensityLabel, L"Intensity: %d%%", g_settings.intensity);
  AppendMenuW(g_menu, MF_STRING | MF_GRAYED, 0, intensityLabel);
  AppendMenuW(g_menu, MF_STRING, ID_INTENSITY_DEC, L"Intensity −");
  AppendMenuW(g_menu, MF_STRING, ID_INTENSITY_INC, L"Intensity +");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_menu, MF_STRING | (g_settings.startWithWindows ? MF_CHECKED : 0), ID_AUTOSTART,
              L"Start with Windows");
  AppendMenuW(g_menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_menu, MF_STRING, ID_EXIT, L"Exit");
}

void ShowContextMenu(HWND hwnd) {
  RebuildMenu();
  POINT pt;
  GetCursorPos(&pt);
  SetForegroundWindow(hwnd);
  TrackPopupMenu(g_menu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, nullptr);
  PostMessageW(hwnd, WM_NULL, 0, 0);
}

void AddTrayIcon(HWND hwnd) {
  g_nid = {};
  g_nid.cbSize = sizeof(g_nid);
  g_nid.hWnd = hwnd;
  g_nid.uID = ID_TRAY;
  g_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  g_nid.uCallbackMessage = WM_TRAY;
  g_nid.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
  wcsncpy_s(g_nid.szTip, L"Phostide", _TRUNCATE);
  Shell_NotifyIconW(NIM_ADD, &g_nid);
}

void RemoveTrayIcon() {
  Shell_NotifyIconW(NIM_DELETE, &g_nid);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE:
      AddTrayIcon(hwnd);
      return 0;
    case WM_TRAY:
      if (lParam == WM_RBUTTONUP || lParam == WM_CONTEXTMENU) {
        ShowContextMenu(hwnd);
      } else if (lParam == WM_LBUTTONUP) {
        g_settings.enabled = !g_settings.enabled;
        phostide::SaveSettings(g_settings);
        PushThemeToTap();
      }
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == ID_ENABLE) {
        g_settings.enabled = !g_settings.enabled;
      } else if (id >= ID_THEME_BASE &&
                 id < ID_THEME_BASE + static_cast<int>(phostide::ThemeId::Count)) {
        g_settings.theme = phostide::ThemeFromIndex(id - ID_THEME_BASE);
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
      PushThemeToTap();
      return 0;
    }
    case WM_WTSSESSION_CHANGE:
      if (g_settings.pauseWhenLocked) {
        if (wParam == WTS_SESSION_LOCK) {
          phostide::ThemeCommand off = {};
          off.apiVersion = phostide::kTapApiVersion;
          off.enabled = 0;
          phostide::SendThemeCommand(off);
        } else if (wParam == WTS_SESSION_UNLOCK) {
          PushThemeToTap();
        }
      }
      return 0;
    case WM_DESTROY:
      RemoveTrayIcon();
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

  g_settings = phostide::LoadSettings();

  WNDCLASSEXW wc{sizeof(wc)};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hi;
  wc.lpszClassName = L"PhostideTray";
  RegisterClassExW(&wc);

  g_hwnd = CreateWindowExW(0, wc.lpszClassName, phostide::kAppName.data(), 0, 0, 0, 0, 0,
                           HWND_MESSAGE, nullptr, hi, nullptr);

  WTSRegisterSessionNotification(g_hwnd, NOTIFY_FOR_THIS_SESSION);

  std::wstring injectError;
  if (!phostide::InjectExplorerTap(&injectError)) {
    MessageBoxW(nullptr, injectError.c_str(), L"Phostide", MB_ICONWARNING | MB_OK);
  }
  PushThemeToTap();

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  // Ask TAP to restore via disabled command before exit.
  phostide::ThemeCommand off{};
  off.apiVersion = phostide::kTapApiVersion;
  off.enabled = 0;
  phostide::SendThemeCommand(off);
  return 0;
}
