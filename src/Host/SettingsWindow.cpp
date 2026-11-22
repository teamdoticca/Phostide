#include "SettingsWindow.h"
#include "resource.h"
#include "../Common/MonitorEnum.h"
#include "../ExplorerTAP/PixelThemes.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>

#include <algorithm>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace phostide {
namespace {

constexpr wchar_t kClassName[] = L"PhostideSettings";

constexpr int kClientW = 720;
constexpr int kClientH = 752;  // Care pause rows; live pause status lives on Monitors
constexpr int kPreviewH = 44;
constexpr UINT_PTR ID_PREVIEW_TIMER = 77;
constexpr UINT kPreviewIntervalMs = 50;

constexpr int IDC_THEME_BASE = 1000;
constexpr int IDC_INTENSITY = 1100;
constexpr int IDC_INTENSITY_VALUE = 1101;
constexpr int IDC_AUTOSTART = 1200;
constexpr int IDC_VERSION = 1300;
constexpr int IDC_UPDATE_STATUS = 1301;
constexpr int IDC_CHECK_UPDATES = 1302;
constexpr int IDC_UPDATE_BTN = 1303;
constexpr int IDC_DIAG_CAPTION = 1310;
constexpr int IDC_COPY_DIAG = 1311;
constexpr int IDC_DIAG_STATUS = 1312;
constexpr int IDC_APPLY = 1400;
constexpr int IDC_CLOSE = 1401;
constexpr int IDC_SUBTITLE = 1502;
constexpr int IDC_TITLE = 1503;
constexpr int IDC_INTENSITY_CAPTION = 1504;
constexpr int IDC_APPEARANCE_CAPTION = 1505;
constexpr int IDC_UPDATES_CAPTION = 1506;
constexpr int IDC_PREVIEW_CAPTION = 1507;
constexpr int IDC_PREVIEW = 1508;
constexpr int IDC_PREVIEW_HINT = 1509;
constexpr int IDC_TAB_TASKBAR = 1510;
constexpr int IDC_TAB_CARE = 1511;
constexpr int IDC_TAB_MONITORS = 1512;
constexpr int IDC_TAB_SYSTEM = 1513;
constexpr int IDC_GENERAL_CAPTION = 1514;
constexpr int IDC_RENDER_CAPTION = 1515;
constexpr int IDC_RENDER_BASE = 1520;  // +0 Auto, +1 GPU, +2 CPU
constexpr int IDC_SCOPE_CAPTION = 1530;
constexpr int IDC_SCOPE_BASE = 1540;  // +0 All, +1..N monitors
constexpr int IDC_MONITORS_CAPTION = 1560;
constexpr int IDC_MONITORS_HINT = 1561;
constexpr int IDC_MONITOR_BASE = 1570;  // +0..N-1 toggle rows
constexpr int IDC_CARE_SUBTITLE = 1620;
constexpr int IDC_CARE_IDLE_CAPTION = 1621;
constexpr int IDC_CARE_IDLE_BASE = 1630;  // Off / 1 / 5 / 10 / 30
constexpr int IDC_CARE_FILL_CAPTION = 1640;
constexpr int IDC_CARE_FILL = 1641;
constexpr int IDC_CARE_FILL_VALUE = 1642;
constexpr int IDC_CARE_CHROME = 1643;
constexpr int IDC_CARE_NUDGE = 1644;
constexpr int IDC_CARE_NIGHT = 1645;
constexpr int IDC_CARE_PAUSE_LOCK = 1646;
constexpr int IDC_CARE_PRESET = 1647;
constexpr int IDC_CARE_HINT = 1648;
constexpr int IDC_CARE_OPTIONS_CAPTION = 1649;
constexpr int IDC_CARE_PAUSE_FS = 1650;
constexpr int IDC_WELCOME_INTRO = 1660;
constexpr int IDC_WELCOME_CAPTION = 1661;
constexpr int IDC_WELCOME_HINT = 1662;

// Pseudo-tab shown only on first run (no tab button): pick OLED displays.
constexpr int kTabWelcome = 4;

constexpr int kMaxScopeChips = 1 + static_cast<int>(kMaxMonitorSlots);  // All + 8
constexpr int kCareIdleChipCount = kOledCareIdleChoiceCount;

constexpr COLORREF kVoid = RGB(0, 0, 0);
constexpr COLORREF kScreen = RGB(17, 17, 17);
constexpr COLORREF kPanel = RGB(28, 28, 30);
constexpr COLORREF kLine = RGB(55, 55, 58);
constexpr COLORREF kInk = RGB(244, 244, 245);
constexpr COLORREF kSoft = RGB(150, 150, 155);
constexpr COLORREF kEmber = RGB(255, 138, 61);
constexpr COLORREF kEmberHot = RGB(255, 160, 96);

SettingsWindowHost g_host{};
HWND g_settingsHwnd = nullptr;
bool g_classRegistered = false;

ThemeId g_draftTheme = ThemeId::Rainbow;
int g_draftIntensity = 45;
bool g_draftAutostart = false;
RenderMode g_draftRenderMode = RenderMode::Auto;
int g_draftCareIdleMinutes = 0;
int g_draftCareFillIntensity = 8;
bool g_draftCareChromeDim = false;
bool g_draftCareClockNudge = false;
bool g_draftCareNightBoost = false;
bool g_draftPauseWhenLocked = true;
bool g_draftPauseWhenFullscreen = true;
// -1 = All monitors; 0..n-1 = attached monitor index.
int g_scopeIndex = -1;
int g_activeTab = 0;  // 0 Taskbar, 1 OLED Care, 2 Monitors, 3 System, 4 Welcome
float g_previewTime = 3.2f;
int g_previewW = 480;
HBITMAP g_previewBmp = nullptr;
void* g_previewBits = nullptr;
int g_previewBmpW = 0;
int g_previewBmpH = 0;

std::vector<AttachedMonitor> g_monitors;
std::vector<bool> g_draftMonitorActive;  // parallel to g_monitors

HFONT g_fontUi = nullptr;
HFONT g_fontSection = nullptr;
HFONT g_fontTitle = nullptr;
HFONT g_fontValue = nullptr;
HBRUSH g_brScreen = nullptr;
HBRUSH g_brPanel = nullptr;

float ThemeBaseTime(ThemeId id) {
  switch (id) {
    case ThemeId::Rainbow:  return 3.2f;
    case ThemeId::SeaWaves: return 5.5f;
    case ThemeId::Weather:  return 8.0f;
    case ThemeId::Space:    return 6.0f;
    case ThemeId::Aurora:   return 4.5f;
    default:                return 3.0f;
  }
}

void EnsureGdi() {
  if (!g_brScreen) g_brScreen = CreateSolidBrush(kScreen);
  if (!g_brPanel) g_brPanel = CreateSolidBrush(kPanel);
  if (!g_fontUi) {
    g_fontUi = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                           OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
  }
  if (!g_fontSection) {
    g_fontSection = CreateFontW(-13, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
  }
  if (!g_fontTitle) {
    g_fontTitle = CreateFontW(-24, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
  }
  if (!g_fontValue) {
    g_fontValue = CreateFontW(-18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
  }
}

HINSTANCE ParentInstance(HWND parent) {
  return reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent, GWLP_HINSTANCE));
}

bool HasPendingUpdate() {
  if (!g_host.settings) return false;
  return !g_host.settings->pendingUpdateTag.empty() &&
         IsVersionNewer(g_host.settings->pendingUpdateTag, kAppVersion);
}

void LoadAppearanceDraft(const MonitorAppearance& a) {
  g_draftTheme = a.theme;
  g_draftIntensity = a.intensity;
  g_previewTime = ThemeBaseTime(g_draftTheme);
}

void SyncDraftFromScope() {
  if (!g_host.settings) return;
  g_draftAutostart = g_host.settings->startWithWindows;
  g_draftRenderMode = g_host.settings->renderMode;
  g_draftCareIdleMinutes = ClampOledCareIdleMinutes(g_host.settings->oledCareIdleMinutes);
  g_draftCareFillIntensity = ClampPercent(g_host.settings->oledCareFillIntensity);
  g_draftCareChromeDim = g_host.settings->oledCareChromeDim;
  g_draftCareClockNudge = g_host.settings->oledCareClockNudge;
  g_draftCareNightBoost = g_host.settings->oledCareNightBoost;
  g_draftPauseWhenLocked = g_host.settings->pauseWhenLocked;
  g_draftPauseWhenFullscreen = g_host.settings->pauseWhenFullscreen;
  if (g_scopeIndex >= 0 && g_scopeIndex < static_cast<int>(g_monitors.size())) {
    LoadAppearanceDraft(
        ResolveMonitorAppearance(*g_host.settings, g_monitors[g_scopeIndex].deviceName));
  } else {
    MonitorAppearance a;
    a.theme = g_host.settings->theme;
    a.intensity = g_host.settings->intensity;
    LoadAppearanceDraft(a);
  }
}

void RefreshMonitorList() {
  g_monitors = EnumerateAttachedMonitors();
  if (g_monitors.size() > kMaxMonitorSlots) g_monitors.resize(kMaxMonitorSlots);
  g_draftMonitorActive.resize(g_monitors.size());
  for (size_t i = 0; i < g_monitors.size(); ++i) {
    const bool disabled =
        g_host.settings && IsMonitorDisabled(*g_host.settings, g_monitors[i].deviceName);
    g_draftMonitorActive[i] = !disabled;
  }
  if (g_scopeIndex >= static_cast<int>(g_monitors.size())) g_scopeIndex = -1;
}

void SyncDraftFromSettings() {
  if (!g_host.settings) return;
  RefreshMonitorList();
  SyncDraftFromScope();
}

void InvalidateRenderRows(HWND hwnd) {
  for (int i = 0; i < 3; ++i) {
    InvalidateRect(GetDlgItem(hwnd, IDC_RENDER_BASE + i), nullptr, TRUE);
  }
}

void InvalidateScopeChips(HWND hwnd) {
  for (int i = 0; i < kMaxScopeChips; ++i) {
    HWND c = GetDlgItem(hwnd, IDC_SCOPE_BASE + i);
    if (c) InvalidateRect(c, nullptr, TRUE);
  }
}

void InvalidateMonitorRows(HWND hwnd) {
  for (int i = 0; i < static_cast<int>(kMaxMonitorSlots); ++i) {
    HWND c = GetDlgItem(hwnd, IDC_MONITOR_BASE + i);
    if (c) InvalidateRect(c, nullptr, TRUE);
  }
}

void UpdateIntensityValue(HWND hwnd) {
  wchar_t buf[32];
  swprintf_s(buf, L"%d%%", g_draftIntensity);
  SetWindowTextW(GetDlgItem(hwnd, IDC_INTENSITY_VALUE), buf);
}

void UpdateCareFillValue(HWND hwnd) {
  wchar_t buf[32];
  swprintf_s(buf, L"%d%%", g_draftCareFillIntensity);
  SetWindowTextW(GetDlgItem(hwnd, IDC_CARE_FILL_VALUE), buf);
}

void InvalidateCareIdleChips(HWND hwnd) {
  for (int i = 0; i < kCareIdleChipCount; ++i) {
    HWND c = GetDlgItem(hwnd, IDC_CARE_IDLE_BASE + i);
    if (c) InvalidateRect(c, nullptr, TRUE);
  }
}

void InvalidateCareChecks(HWND hwnd) {
  for (int id : {IDC_CARE_CHROME, IDC_CARE_NUDGE, IDC_CARE_NIGHT, IDC_CARE_PAUSE_LOCK,
                 IDC_CARE_PAUSE_FS, IDC_AUTOSTART}) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) InvalidateRect(c, nullptr, TRUE);
  }
}

const wchar_t* CareIdleChipLabel(int index) {
  switch (index) {
    case 0: return L"Off";
    case 1: return L"1m";
    case 2: return L"5m";
    case 3: return L"10m";
    case 4: return L"30m";
    default: return L"";
  }
}

void InvalidateThemeRows(HWND hwnd);
void RefreshPreview(HWND hwnd);

void ApplyOledEverydayPreset(HWND hwnd) {
  // Care-only: never touch Taskbar theme / intensity (user's active look stays).
  g_draftCareIdleMinutes = 5;
  g_draftCareFillIntensity = 5;
  g_draftCareChromeDim = true;
  g_draftCareClockNudge = true;
  g_draftCareNightBoost = true;
  SendDlgItemMessageW(hwnd, IDC_CARE_FILL, TBM_SETPOS, TRUE,
                      static_cast<LPARAM>(g_draftCareFillIntensity));
  UpdateCareFillValue(hwnd);
  InvalidateCareIdleChips(hwnd);
  InvalidateCareChecks(hwnd);
}

void InvalidateThemeRows(HWND hwnd) {
  for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i) {
    InvalidateRect(GetDlgItem(hwnd, IDC_THEME_BASE + i), nullptr, TRUE);
  }
}

bool EnsurePreviewDib(int w, int h) {
  if (g_previewBmp && g_previewBmpW == w && g_previewBmpH == h && g_previewBits) {
    return true;
  }
  if (g_previewBmp) {
    DeleteObject(g_previewBmp);
    g_previewBmp = nullptr;
    g_previewBits = nullptr;
  }
  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h;  // top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  g_previewBmp = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &g_previewBits, nullptr, 0);
  if (!g_previewBmp || !g_previewBits) {
    g_previewBmp = nullptr;
    g_previewBits = nullptr;
    g_previewBmpW = g_previewBmpH = 0;
    return false;
  }
  g_previewBmpW = w;
  g_previewBmpH = h;
  return true;
}

void RenderPreviewBitmap() {
  if (g_previewW < 64) g_previewW = 64;
  const int w = g_previewW;
  const int h = kPreviewH;
  const int stride = w * 4;
  if (!EnsurePreviewDib(w, h)) return;

  auto* pixels = static_cast<uint8_t*>(g_previewBits);
  PaintContext ctx;
  ctx.theme = g_draftTheme;
  ctx.intensity = static_cast<float>(g_draftIntensity) / 100.f;
  ctx.timeSeconds = g_previewTime;
  ctx.edge = TaskbarEdge::Bottom;
  ctx.vertical = false;
  ctx.thickness = h;
  ctx.length = w;
  PaintThemePixels(ctx, pixels, w, h, stride);

  for (int i = 0; i < 5; ++i) {
    const int cx = 18 + i * 22;
    const int cy = h / 2;
    const int r = 4;
    for (int py = cy - r; py <= cy + r; ++py) {
      for (int px = cx - r; px <= cx + r; ++px) {
        if (px < 0 || px >= w || py < 0 || py >= h) continue;
        if ((px - cx) * (px - cx) + (py - cy) * (py - cy) > r * r) continue;
        uint8_t* p = pixels + static_cast<size_t>(py) * stride + static_cast<size_t>(px) * 4;
        p[0] = static_cast<uint8_t>(p[0] * 0.35f + 220 * 0.65f);
        p[1] = static_cast<uint8_t>(p[1] * 0.35f + 220 * 0.65f);
        p[2] = static_cast<uint8_t>(p[2] * 0.35f + 220 * 0.65f);
        p[3] = 255;
      }
    }
  }
}

void RefreshPreview(HWND hwnd) {
  RenderPreviewBitmap();
  InvalidateRect(GetDlgItem(hwnd, IDC_PREVIEW), nullptr, FALSE);
}

void RefreshUpdateControls(HWND hwnd) {
  if (!hwnd || !g_host.settings) return;

  wchar_t version[80];
  swprintf_s(version, L"Installed version  v%.*s", static_cast<int>(kAppVersion.size()),
             kAppVersion.data());
  SetWindowTextW(GetDlgItem(hwnd, IDC_VERSION), version);

  const bool checking = g_host.updateCheckRunning && *g_host.updateCheckRunning;
  const bool installing = g_host.updateInstallRunning && *g_host.updateInstallRunning;
  wchar_t status[128];
  if (checking && !installing) {
    wcsncpy_s(status, L"Checking for updates...", _TRUNCATE);
  } else if (g_host.updateStatusText && !g_host.updateStatusText->empty()) {
    wcsncpy_s(status, g_host.updateStatusText->c_str(), _TRUNCATE);
  } else if (HasPendingUpdate()) {
    swprintf_s(status, L"Update available  %s", g_host.settings->pendingUpdateTag.c_str());
  } else {
    wcsncpy_s(status, L"You're up to date", _TRUNCATE);
  }
  SetWindowTextW(GetDlgItem(hwnd, IDC_UPDATE_STATUS), status);

  EnableWindow(GetDlgItem(hwnd, IDC_CHECK_UPDATES), (checking || installing) ? FALSE : TRUE);
  EnableWindow(GetDlgItem(hwnd, IDC_UPDATE_BTN),
               (!checking && !installing && HasPendingUpdate()) ? TRUE : FALSE);
  InvalidateRect(GetDlgItem(hwnd, IDC_UPDATE_BTN), nullptr, TRUE);
  InvalidateRect(GetDlgItem(hwnd, IDC_UPDATE_STATUS), nullptr, TRUE);
}

void UpdateMonitorsHint(HWND hwnd);

bool HostFullscreenOnHash(uint64_t deviceHash) {
  if (!deviceHash || !g_host.fullscreenHashes || !g_host.fullscreenHashCount) return false;
  const uint32_t n = *g_host.fullscreenHashCount;
  for (uint32_t i = 0; i < n && i < kMaxMonitorSlots; ++i) {
    if (g_host.fullscreenHashes[i] == deviceHash) return true;
  }
  return false;
}

const wchar_t* MonitorPaintStatusLabel(int index, bool draftOn) {
  if (!draftOn) return L"off";
  if (!g_host.settings) return L"";
  if (!g_host.settings->enabled) return L"paused";
  if (g_host.settings->pauseWhenLocked && g_host.sessionLocked && *g_host.sessionLocked) {
    return L"locked";
  }
  if (index >= 0 && index < static_cast<int>(g_monitors.size()) &&
      g_host.settings->pauseWhenFullscreen && HostFullscreenOnHash(g_monitors[index].deviceHash)) {
    return L"fullscreen";
  }
  return L"painting";
}

void RefreshPauseStatus(HWND hwnd) {
  if (!hwnd) return;
  // Live pause state is shown per row on the Monitors tab.
  InvalidateMonitorRows(hwnd);
  UpdateMonitorsHint(hwnd);
}

void UpdateMonitorsHint(HWND hwnd) {
  HWND hint = GetDlgItem(hwnd, IDC_MONITORS_HINT);
  if (!hint) return;
  bool anyOn = false;
  for (bool on : g_draftMonitorActive) {
    if (on) {
      anyOn = true;
      break;
    }
  }
  if (!anyOn) {
    SetWindowTextW(hint, L"All displays are off - Phostide will not paint any taskbar.");
    return;
  }
  if (g_host.settings && !g_host.settings->enabled) {
    if (g_host.pauseText && !g_host.pauseText->empty()) {
      const std::wstring text = *g_host.pauseText + L" — all bars soft-paused (tray left-click resumes).";
      SetWindowTextW(hint, text.c_str());
    } else {
      SetWindowTextW(hint, L"Enable is off — all bars soft-paused (tray left-click resumes).");
    }
    return;
  }
  if (g_host.settings && g_host.settings->pauseWhenLocked && g_host.sessionLocked &&
      *g_host.sessionLocked) {
    SetWindowTextW(hint, L"Session locked — themes paused on every display.");
    return;
  }
  SetWindowTextW(hint,
                 L"On/off per display. Status shows painting, fullscreen pause, or locked.");
}

void RelayoutCare(HWND hwnd) {
  if (g_activeTab != 1) return;
  const int pad = 24;
  const int contentW = kClientW - pad * 2;
  const int footerY = kClientH - 52;

  RECT tabRc{};
  GetWindowRect(GetDlgItem(hwnd, IDC_TAB_CARE), &tabRc);
  POINT tabBottom{tabRc.left, tabRc.bottom};
  ScreenToClient(hwnd, &tabBottom);
  int y = tabBottom.y + 24;  // match Taskbar air under tabs

  auto place = [&](int id, int x, int yy, int w, int h) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) SetWindowPos(c, nullptr, x, yy, w, h, SWP_NOZORDER | SWP_NOACTIVATE);
  };

  place(IDC_CARE_SUBTITLE, pad, y, contentW, 20);
  y += 30;

  place(IDC_CARE_IDLE_CAPTION, pad, y, contentW, 16);
  y += 20;
  {
    const int chipGap = 10;
    const int chipH = 36;
    const int chipW = (contentW - chipGap * (kCareIdleChipCount - 1)) / kCareIdleChipCount;
    for (int i = 0; i < kCareIdleChipCount; ++i) {
      place(IDC_CARE_IDLE_BASE + i, pad + i * (chipW + chipGap), y, chipW, chipH);
    }
    y += chipH + 20;
  }

  place(IDC_CARE_FILL_CAPTION, pad, y, contentW - 70, 16);
  place(IDC_CARE_FILL_VALUE, pad + contentW - 64, y - 2, 64, 22);
  y += 22;
  place(IDC_CARE_FILL, pad - 4, y, contentW + 8, 30);
  y += 38;

  place(IDC_CARE_OPTIONS_CAPTION, pad, y, contentW, 16);
  y += 20;
  const int rowH = 38;
  const int rowGap = 8;
  const int optIds[] = {IDC_CARE_CHROME, IDC_CARE_NUDGE, IDC_CARE_NIGHT, IDC_CARE_PAUSE_LOCK,
                        IDC_CARE_PAUSE_FS};
  for (int id : optIds) {
    place(id, pad, y, contentW, rowH);
    y += rowH + rowGap;
  }

  // Preset + hint always above the footer (never push Close/Apply off-screen).
  const int presetH = 40;
  const int hintH = 18;
  const int hintGap = 8;
  const int footerClearance = 16;
  int bottomY = footerY - footerClearance - hintH - hintGap - presetH;
  if (bottomY < y + 10) bottomY = y + 10;
  if (bottomY + presetH + hintGap + hintH > footerY - footerClearance) {
    bottomY = footerY - footerClearance - hintH - hintGap - presetH;
  }
  place(IDC_CARE_PRESET, pad, bottomY, contentW, presetH);
  place(IDC_CARE_HINT, pad, bottomY + presetH + hintGap, contentW, hintH);
}

void RelayoutScopeAndMonitors(HWND hwnd) {
  const int pad = 24;
  const int contentW = kClientW - pad * 2;
  const int chipH = 32;
  const int chipGap = 8;
  const int allW = 56;
  const int numW = 44;

  // Scope chips: All + one per attached monitor.
  int sx = pad;
  const int scopeY = [&]() {
    RECT rc{};
    GetWindowRect(GetDlgItem(hwnd, IDC_SCOPE_CAPTION), &rc);
    POINT pt{rc.left, rc.bottom};
    ScreenToClient(hwnd, &pt);
    return pt.y + 8;
  }();

  for (int i = 0; i < kMaxScopeChips; ++i) {
    HWND c = GetDlgItem(hwnd, IDC_SCOPE_BASE + i);
    if (!c) continue;
    const bool show = (i == 0) || (i - 1 < static_cast<int>(g_monitors.size()));
    ShowWindow(c, (g_activeTab == 0 && show) ? SW_SHOW : SW_HIDE);
    if (!show) continue;
    const int w = (i == 0) ? allW : numW;
    SetWindowPos(c, nullptr, sx, scopeY, w, chipH, SWP_NOZORDER | SWP_NOACTIVATE);
    if (i == 0) {
      SetWindowTextW(c, L"All");
    } else {
      wchar_t label[16];
      swprintf_s(label, L"%d", i);
      SetWindowTextW(c, label);
    }
    sx += w + chipGap;
  }

  // Monitor activation rows (Monitors tab, and reused by the first-run Welcome).
  const bool welcome = g_activeTab == kTabWelcome;
  RECT captionRc{};
  GetWindowRect(GetDlgItem(hwnd, welcome ? IDC_WELCOME_CAPTION : IDC_MONITORS_CAPTION),
                &captionRc);
  POINT capt{captionRc.left, captionRc.bottom};
  ScreenToClient(hwnd, &capt);
  int my = capt.y + 14;
  const int rowH = 40;
  for (int i = 0; i < static_cast<int>(kMaxMonitorSlots); ++i) {
    HWND c = GetDlgItem(hwnd, IDC_MONITOR_BASE + i);
    if (!c) continue;
    const bool show = i < static_cast<int>(g_monitors.size());
    ShowWindow(c, ((g_activeTab == 2 || welcome) && show) ? SW_SHOW : SW_HIDE);
    if (!show) continue;
    SetWindowPos(c, nullptr, pad, my, contentW, rowH, SWP_NOZORDER | SWP_NOACTIVATE);
    my += rowH + 10;
  }
  if (HWND hint = GetDlgItem(hwnd, IDC_WELCOME_HINT)) {
    SetWindowPos(hint, nullptr, pad, my + 6, contentW, 44, SWP_NOZORDER | SWP_NOACTIVATE);
  }
  UpdateMonitorsHint(hwnd);
}

void LoadControlsFromDraft(HWND hwnd) {
  SendDlgItemMessageW(hwnd, IDC_INTENSITY, TBM_SETPOS, TRUE,
                      static_cast<LPARAM>(g_draftIntensity));
  UpdateIntensityValue(hwnd);
  SendDlgItemMessageW(hwnd, IDC_CARE_FILL, TBM_SETPOS, TRUE,
                      static_cast<LPARAM>(g_draftCareFillIntensity));
  UpdateCareFillValue(hwnd);
  CheckDlgButton(hwnd, IDC_AUTOSTART, g_draftAutostart ? BST_CHECKED : BST_UNCHECKED);
  InvalidateThemeRows(hwnd);
  InvalidateRenderRows(hwnd);
  InvalidateScopeChips(hwnd);
  InvalidateMonitorRows(hwnd);
  InvalidateCareIdleChips(hwnd);
  InvalidateCareChecks(hwnd);
  RefreshUpdateControls(hwnd);
  RefreshPauseStatus(hwnd);
  RelayoutScopeAndMonitors(hwnd);
  RefreshPreview(hwnd);
}

void ReadDraftFromControls(HWND hwnd) {
  // Only sample the trackbar for the active tab. Hidden trackbars have been seen
  // to disagree with draft after Care layout/preset, which previously saved Idle
  // fill % over Taskbar intensity (themes looked "not applied" at ~5%).
  if (g_activeTab == 0) {
    g_draftIntensity =
        static_cast<int>(SendDlgItemMessageW(hwnd, IDC_INTENSITY, TBM_GETPOS, 0, 0));
    g_draftIntensity = ClampPercent(g_draftIntensity);
  } else if (g_activeTab == 1) {
    g_draftCareFillIntensity =
        static_cast<int>(SendDlgItemMessageW(hwnd, IDC_CARE_FILL, TBM_GETPOS, 0, 0));
    g_draftCareFillIntensity = ClampPercent(g_draftCareFillIntensity);
  }
  (void)hwnd;
}

void ApplyDraft(HWND hwnd) {
  if (!g_host.settings || !g_host.onApplyAppearance) return;
  ReadDraftFromControls(hwnd);
  g_host.settings->startWithWindows = g_draftAutostart;
  g_host.settings->renderMode = g_draftRenderMode;
  g_host.settings->oledCareIdleMinutes = ClampOledCareIdleMinutes(g_draftCareIdleMinutes);
  g_host.settings->oledCareFillIntensity = ClampPercent(g_draftCareFillIntensity);
  g_host.settings->oledCareChromeDim = g_draftCareChromeDim;
  g_host.settings->oledCareClockNudge = g_draftCareClockNudge;
  g_host.settings->oledCareNightBoost = g_draftCareNightBoost;
  g_host.settings->pauseWhenLocked = g_draftPauseWhenLocked;
  g_host.settings->pauseWhenFullscreen = g_draftPauseWhenFullscreen;

  MonitorAppearance appearance;
  appearance.theme = g_draftTheme;
  appearance.intensity = g_draftIntensity;

  if (g_scopeIndex < 0) {
    ApplyAppearanceToAllAttached(*g_host.settings, appearance);
  } else if (g_scopeIndex < static_cast<int>(g_monitors.size())) {
    UpsertMonitorAppearance(*g_host.settings, g_monitors[g_scopeIndex].deviceName, appearance);
  }

  for (size_t i = 0; i < g_monitors.size(); ++i) {
    SetMonitorDisabled(*g_host.settings, g_monitors[i].deviceName, !g_draftMonitorActive[i]);
  }

  g_host.onApplyAppearance();
}

HWND MkStatic(HWND parent, int id, const wchar_t* text, int x, int y, int w, int height,
              HFONT font, DWORD style = SS_LEFT) {
  HWND ctrl =
      CreateWindowExW(0, L"STATIC", text, WS_CHILD | WS_VISIBLE | style, x, y, w, height,
                      parent,
                      id > 0 ? reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)) : nullptr,
                      ParentInstance(parent), nullptr);
  SendMessageW(ctrl, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
  return ctrl;
}

HWND MkButton(HWND parent, int id, const wchar_t* text, int x, int y, int w, int height,
              DWORD style = BS_PUSHBUTTON) {
  HWND ctrl = CreateWindowExW(0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | WS_TABSTOP | style,
                              x, y, w, height, parent,
                              reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),
                              ParentInstance(parent), nullptr);
  SendMessageW(ctrl, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontUi), TRUE);
  SetWindowTheme(ctrl, L"", L"");
  return ctrl;
}

void EnableDarkTitleBar(HWND hwnd) {
  BOOL useDark = TRUE;
  DwmSetWindowAttribute(hwnd, 20, &useDark, sizeof(useDark));
}

// backdrop: when set, paint the whole rect first so the pixels outside the rounded
// corners match the window instead of the default (white) button background.
void FillRoundRect(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int radius,
                   COLORREF backdrop = CLR_INVALID) {
  if (backdrop != CLR_INVALID) {
    HBRUSH bg = CreateSolidBrush(backdrop);
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
  }
  HBRUSH br = CreateSolidBrush(fill);
  HPEN pen = CreatePen(PS_SOLID, 1, border);
  HGDIOBJ oldBr = SelectObject(hdc, br);
  HGDIOBJ oldPen = SelectObject(hdc, pen);
  RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
  SelectObject(hdc, oldBr);
  SelectObject(hdc, oldPen);
  DeleteObject(br);
  DeleteObject(pen);
}

void DrawPrimaryButton(DRAWITEMSTRUCT* dis, const wchar_t* label, bool enabled) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = !enabled ? RGB(60, 60, 64) : (pressed ? kEmberHot : kEmber);
  const COLORREF text = !enabled ? kSoft : kVoid;
  FillRoundRect(dis->hDC, dis->rcItem, fill, fill, 10, kScreen);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, text);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontSection));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawGhostButton(DRAWITEMSTRUCT* dis, const wchar_t* label, bool enabled) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  FillRoundRect(dis->hDC, dis->rcItem, pressed ? RGB(40, 40, 44) : kPanel, kLine, 10, kScreen);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, enabled ? kInk : kSoft);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontUi));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawTabButton(DRAWITEMSTRUCT* dis, const wchar_t* label, bool active) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = active ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = active ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10, kScreen);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, active ? kInk : kSoft);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontSection));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawChip(DRAWITEMSTRUCT* dis, const wchar_t* label, bool selected, bool dimmed) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = selected ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = selected ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10, kScreen);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, dimmed ? RGB(90, 90, 94) : (selected ? kInk : kSoft));
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontSection));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawThemeRow(DRAWITEMSTRUCT* dis, int themeIndex) {
  const bool selected = g_draftTheme == ThemeFromIndex(themeIndex);
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = selected ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = selected ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10, kScreen);
  if (selected) {
    RECT bar = dis->rcItem;
    bar.right = bar.left + 4;
    HBRUSH ember = CreateSolidBrush(kEmber);
    FillRect(dis->hDC, &bar, ember);
    DeleteObject(ember);
  }
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, kInk);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontUi));
  RECT textRc = dis->rcItem;
  textRc.left += 14;
  DrawTextW(dis->hDC, ThemeName(ThemeFromIndex(themeIndex)), -1, &textRc,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawRenderRow(DRAWITEMSTRUCT* dis, int modeIndex) {
  const RenderMode mode = RenderModeFromIndex(modeIndex);
  const bool selected = g_draftRenderMode == mode;
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = selected ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = selected ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10, kScreen);
  if (selected) {
    RECT bar = dis->rcItem;
    bar.right = bar.left + 4;
    HBRUSH ember = CreateSolidBrush(kEmber);
    FillRect(dis->hDC, &bar, ember);
    DeleteObject(ember);
  }
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, kInk);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontUi));
  RECT textRc = dis->rcItem;
  textRc.left += 14;
  DrawTextW(dis->hDC, RenderModeName(mode), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawCheckRow(DRAWITEMSTRUCT* dis, const wchar_t* label, bool checked) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  FillRoundRect(dis->hDC, dis->rcItem, pressed ? RGB(34, 34, 36) : kPanel, kLine, 10, kScreen);

  RECT box = dis->rcItem;
  box.left += 14;
  box.top += (box.bottom - box.top - 16) / 2;
  box.right = box.left + 16;
  box.bottom = box.top + 16;
  FillRoundRect(dis->hDC, box, checked ? kEmber : kScreen, checked ? kEmber : kLine, 4);
  if (checked) {
    HPEN pen = CreatePen(PS_SOLID, 2, kVoid);
    HGDIOBJ oldPen = SelectObject(dis->hDC, pen);
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    MoveToEx(dis->hDC, cx - 4, cy, nullptr);
    LineTo(dis->hDC, cx - 1, cy + 3);
    LineTo(dis->hDC, cx + 5, cy - 4);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(pen);
  }

  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, kInk);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontUi));
  RECT textRc = dis->rcItem;
  textRc.left += 42;
  DrawTextW(dis->hDC, label, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
  SelectObject(dis->hDC, old);
}

void DrawMonitorRow(DRAWITEMSTRUCT* dis, int index) {
  if (index < 0 || index >= static_cast<int>(g_monitors.size())) return;
  const auto& mon = g_monitors[index];
  const bool on = index < static_cast<int>(g_draftMonitorActive.size()) &&
                  g_draftMonitorActive[index];
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  FillRoundRect(dis->hDC, dis->rcItem, pressed ? RGB(34, 34, 36) : kPanel, kLine, 10, kScreen);

  RECT box = dis->rcItem;
  box.left += 14;
  box.top += (box.bottom - box.top - 18) / 2;
  box.right = box.left + 18;
  box.bottom = box.top + 18;
  FillRoundRect(dis->hDC, box, on ? kEmber : kScreen, on ? kEmber : kLine, 4);
  if (on) {
    HPEN pen = CreatePen(PS_SOLID, 2, kVoid);
    HGDIOBJ oldPen = SelectObject(dis->hDC, pen);
    const int cx = (box.left + box.right) / 2;
    const int cy = (box.top + box.bottom) / 2;
    MoveToEx(dis->hDC, cx - 4, cy, nullptr);
    LineTo(dis->hDC, cx - 1, cy + 3);
    LineTo(dis->hDC, cx + 5, cy - 4);
    SelectObject(dis->hDC, oldPen);
    DeleteObject(pen);
  }

  const wchar_t* status = MonitorPaintStatusLabel(index, on);
  const bool statusHot =
      wcscmp(status, L"fullscreen") == 0 || wcscmp(status, L"locked") == 0 ||
      wcscmp(status, L"paused") == 0;

  wchar_t line[128];
  if (mon.primary) {
    swprintf_s(line, L"Monitor %d  ·  %d×%d  ·  Primary", index + 1, mon.width, mon.height);
  } else {
    swprintf_s(line, L"Monitor %d  ·  %d×%d", index + 1, mon.width, mon.height);
  }

  SetBkMode(dis->hDC, TRANSPARENT);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontUi));
  RECT textRc = dis->rcItem;
  textRc.left += 44;
  textRc.right -= 110;
  SetTextColor(dis->hDC, on ? kInk : kSoft);
  DrawTextW(dis->hDC, line, -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);

  RECT statusRc = dis->rcItem;
  statusRc.right -= 16;
  statusRc.left = statusRc.right - 96;
  SetTextColor(dis->hDC, statusHot ? kEmber : kSoft);
  DrawTextW(dis->hDC, status, -1, &statusRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  SelectObject(dis->hDC, old);
}

void DrawPreview(DRAWITEMSTRUCT* dis) {
  const int cw = dis->rcItem.right - dis->rcItem.left;
  const int ch = dis->rcItem.bottom - dis->rcItem.top;
  HDC mem = CreateCompatibleDC(dis->hDC);
  HBITMAP back = CreateCompatibleBitmap(dis->hDC, cw, ch);
  HGDIOBJ oldBmp = SelectObject(mem, back);

  RECT local{0, 0, cw, ch};
  FillRoundRect(mem, local, kVoid, kLine, 12);

  if (g_previewBmp) {
    HDC src = CreateCompatibleDC(dis->hDC);
    HGDIOBJ oldSrc = SelectObject(src, g_previewBmp);
    RECT inner = local;
    InflateRect(&inner, -3, -3);
    const int dw = inner.right - inner.left;
    const int dh = inner.bottom - inner.top;
    SetStretchBltMode(mem, COLORONCOLOR);
    StretchBlt(mem, inner.left, inner.top, dw, dh, src, 0, 0, g_previewBmpW, g_previewBmpH,
               SRCCOPY);
    SelectObject(src, oldSrc);
    DeleteDC(src);
  }

  BitBlt(dis->hDC, dis->rcItem.left, dis->rcItem.top, cw, ch, mem, 0, 0, SRCCOPY);
  SelectObject(mem, oldBmp);
  DeleteObject(back);
  DeleteDC(mem);
}

void UpdateWelcomeIntro(HWND hwnd) {
  if (!g_host.settings) return;
  const int idle = ClampOledCareIdleMinutes(g_host.settings->oledCareIdleMinutes);
  wchar_t care[96];
  if (idle > 0) {
    swprintf_s(care, L"OLED Care dims a bar after %d minute%s without use", idle,
               idle == 1 ? L"" : L"s");
  } else {
    wcsncpy_s(care, L"OLED Care is off", _TRUNCATE);
  }
  wchar_t text[320];
  swprintf_s(text,
             L"Your taskbar is now painted with %s at %d%%, and %s. "
             L"Leave checked only the displays that are OLED - unchecked ones keep "
             L"the normal Windows taskbar.",
             ThemeName(g_host.settings->theme), ClampPercent(g_host.settings->intensity), care);
  SetWindowTextW(GetDlgItem(hwnd, IDC_WELCOME_INTRO), text);
}

void ShowTab(HWND hwnd, int tab) {
  g_activeTab = tab;
  const BOOL taskbar = (tab == 0) ? TRUE : FALSE;
  const BOOL care = (tab == 1) ? TRUE : FALSE;
  const BOOL monitors = (tab == 2) ? TRUE : FALSE;
  const BOOL system = (tab == 3) ? TRUE : FALSE;
  const BOOL welcome = (tab == kTabWelcome) ? TRUE : FALSE;

  // Welcome replaces the tab row with its own title; footer buttons relabel.
  for (int id : {IDC_TAB_TASKBAR, IDC_TAB_CARE, IDC_TAB_MONITORS, IDC_TAB_SYSTEM}) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, welcome ? SW_HIDE : SW_SHOW);
  }
  for (int id : {IDC_WELCOME_INTRO, IDC_WELCOME_CAPTION, IDC_WELCOME_HINT}) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, welcome ? SW_SHOW : SW_HIDE);
  }
  SetWindowTextW(GetDlgItem(hwnd, IDC_TITLE), welcome ? L"Welcome to Phostide" : L"Settings");
  if (welcome) UpdateWelcomeIntro(hwnd);
  InvalidateRect(GetDlgItem(hwnd, IDC_APPLY), nullptr, TRUE);
  InvalidateRect(GetDlgItem(hwnd, IDC_CLOSE), nullptr, TRUE);

  const int taskbarIds[] = {
      IDC_SUBTITLE,          IDC_SCOPE_CAPTION,      IDC_PREVIEW_CAPTION,    IDC_PREVIEW,
      IDC_PREVIEW_HINT,      IDC_APPEARANCE_CAPTION,  IDC_INTENSITY_CAPTION,  IDC_INTENSITY_VALUE,
      IDC_INTENSITY,
  };
  for (int id : taskbarIds) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, taskbar ? SW_SHOW : SW_HIDE);
  }
  for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i) {
    HWND c = GetDlgItem(hwnd, IDC_THEME_BASE + i);
    if (c) ShowWindow(c, taskbar ? SW_SHOW : SW_HIDE);
  }

  const int careIds[] = {
      IDC_CARE_SUBTITLE,       IDC_CARE_IDLE_CAPTION, IDC_CARE_FILL_CAPTION, IDC_CARE_FILL_VALUE,
      IDC_CARE_FILL,           IDC_CARE_OPTIONS_CAPTION, IDC_CARE_CHROME,    IDC_CARE_NUDGE,
      IDC_CARE_NIGHT,          IDC_CARE_PAUSE_LOCK,   IDC_CARE_PAUSE_FS,     IDC_CARE_PRESET,
      IDC_CARE_HINT,
  };
  for (int id : careIds) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, care ? SW_SHOW : SW_HIDE);
  }
  for (int i = 0; i < kCareIdleChipCount; ++i) {
    HWND c = GetDlgItem(hwnd, IDC_CARE_IDLE_BASE + i);
    if (c) ShowWindow(c, care ? SW_SHOW : SW_HIDE);
  }

  const int monitorIds[] = {IDC_MONITORS_CAPTION, IDC_MONITORS_HINT};
  for (int id : monitorIds) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, monitors ? SW_SHOW : SW_HIDE);
  }

  const int systemIds[] = {
      IDC_GENERAL_CAPTION, IDC_AUTOSTART,      IDC_RENDER_CAPTION,  IDC_RENDER_BASE,
      IDC_RENDER_BASE + 1, IDC_RENDER_BASE + 2, IDC_UPDATES_CAPTION, IDC_VERSION,
      IDC_UPDATE_STATUS,  IDC_CHECK_UPDATES, IDC_UPDATE_BTN,   IDC_DIAG_CAPTION,
      IDC_COPY_DIAG,      IDC_DIAG_STATUS,
  };
  for (int id : systemIds) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, system ? SW_SHOW : SW_HIDE);
  }

  RelayoutScopeAndMonitors(hwnd);
  RelayoutCare(hwnd);

  InvalidateRect(GetDlgItem(hwnd, IDC_TAB_TASKBAR), nullptr, FALSE);
  InvalidateRect(GetDlgItem(hwnd, IDC_TAB_CARE), nullptr, FALSE);
  InvalidateRect(GetDlgItem(hwnd, IDC_TAB_MONITORS), nullptr, FALSE);
  InvalidateRect(GetDlgItem(hwnd, IDC_TAB_SYSTEM), nullptr, FALSE);

  if (tab == 0) {
    SetTimer(hwnd, ID_PREVIEW_TIMER, kPreviewIntervalMs, nullptr);
    RefreshPreview(hwnd);
  } else {
    KillTimer(hwnd, ID_PREVIEW_TIMER);
  }
}

void BuildChildren(HWND hwnd) {
  EnsureGdi();
  const int pad = 24;
  const int contentW = kClientW - pad * 2;
  g_previewW = contentW - 6;
  int y = 14;

  MkStatic(hwnd, IDC_TITLE, L"Settings", pad, y, contentW, 32, g_fontTitle);
  y += 44;  // leave room for title descenders (g) before the tab row

  const int tabGap = 6;
  const int tabW = (contentW - tabGap * 3) / 4;
  MkButton(hwnd, IDC_TAB_TASKBAR, L"Taskbar", pad, y, tabW, 32, BS_OWNERDRAW);
  MkButton(hwnd, IDC_TAB_CARE, L"OLED Care", pad + tabW + tabGap, y, tabW, 32, BS_OWNERDRAW);
  MkButton(hwnd, IDC_TAB_MONITORS, L"Monitors", pad + (tabW + tabGap) * 2, y, tabW, 32,
           BS_OWNERDRAW);
  MkButton(hwnd, IDC_TAB_SYSTEM, L"System", pad + (tabW + tabGap) * 3, y, tabW, 32,
           BS_OWNERDRAW);
  y += 56;  // air between tabs and first content line on every tab

  // --- Taskbar tab ---
  MkStatic(hwnd, IDC_SUBTITLE, L"Preview the taskbar, then Apply when it looks right.", pad,
           y, contentW, 20, g_fontUi);
  int ty = y + 28;

  MkStatic(hwnd, IDC_SCOPE_CAPTION, L"APPLY TO", pad, ty, contentW, 14, g_fontSection);
  ty += 22;
  for (int i = 0; i < kMaxScopeChips; ++i) {
    MkButton(hwnd, IDC_SCOPE_BASE + i, L"", pad, ty, 44, 30, BS_OWNERDRAW);
  }
  ty += 42;

  MkStatic(hwnd, IDC_PREVIEW_CAPTION, L"TASKBAR PREVIEW", pad, ty, contentW, 14,
           g_fontSection);
  ty += 20;
  CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY, pad,
                  ty, contentW, kPreviewH + 8, hwnd,
                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PREVIEW)),
                  ParentInstance(hwnd), nullptr);
  ty += kPreviewH + 14;
  MkStatic(hwnd, IDC_PREVIEW_HINT,
           L"Live preview - changes apply only when you click Apply.", pad, ty, contentW, 18,
           g_fontUi);
  ty += 32;

  const int colGap = 20;
  const int leftW = (contentW - colGap) * 48 / 100;
  const int rightW = contentW - colGap - leftW;
  const int leftX = pad;
  const int rightX = pad + leftW + colGap;

  MkStatic(hwnd, IDC_APPEARANCE_CAPTION, L"THEME", leftX, ty, leftW, 16, g_fontSection);
  int ly = ty + 28;  // clear gap under THEME caption before first chip
  const int themeH = 32;
  const int themeGap = 8;
  for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i) {
    MkButton(hwnd, IDC_THEME_BASE + i, ThemeName(ThemeFromIndex(i)), leftX, ly, leftW,
             themeH, BS_OWNERDRAW);
    ly += themeH + themeGap;
  }

  MkStatic(hwnd, IDC_INTENSITY_CAPTION, L"INTENSITY", rightX, ty, rightW - 70, 16,
           g_fontSection);
  MkStatic(hwnd, IDC_INTENSITY_VALUE, L"0%", rightX + rightW - 64, ty - 2, 64, 20, g_fontValue,
           SS_RIGHT);
  int ry = ty + 28;
  HWND track = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_NOTICKS,
                               rightX - 4, ry, rightW + 8, 28, hwnd,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INTENSITY)),
                               ParentInstance(hwnd), nullptr);
  SendMessageW(track, TBM_SETRANGEMIN, FALSE, 0);
  SendMessageW(track, TBM_SETRANGEMAX, FALSE, 100);
  SendMessageW(track, TBM_SETPAGESIZE, 0, 5);
  SetWindowTheme(track, L"", L"");
  (void)ry;

  // --- OLED Care tab (single column — use full height, not a cramped top cluster) ---
  const int cy = y;
  MkStatic(hwnd, IDC_CARE_SUBTITLE,
           L"Per-display idle follows the cursor. Optional chrome care is experimental.",
           pad, cy, contentW, 20, g_fontUi);

  MkStatic(hwnd, IDC_CARE_IDLE_CAPTION, L"IDLE AFTER", pad, cy, contentW, 16, g_fontSection);
  for (int i = 0; i < kCareIdleChipCount; ++i) {
    MkButton(hwnd, IDC_CARE_IDLE_BASE + i, CareIdleChipLabel(i), pad, cy, 80, 36, BS_OWNERDRAW);
  }

  MkStatic(hwnd, IDC_CARE_FILL_CAPTION, L"IDLE FILL INTENSITY", pad, cy, contentW - 70, 16,
           g_fontSection);
  MkStatic(hwnd, IDC_CARE_FILL_VALUE, L"0%", pad + contentW - 64, cy, 64, 22, g_fontValue,
           SS_RIGHT);
  HWND careTrack =
      CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                      WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_NOTICKS, pad - 4, cy,
                      contentW + 8, 30, hwnd,
                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_CARE_FILL)),
                      ParentInstance(hwnd), nullptr);
  SendMessageW(careTrack, TBM_SETRANGEMIN, FALSE, 0);
  SendMessageW(careTrack, TBM_SETRANGEMAX, FALSE, 100);
  SendMessageW(careTrack, TBM_SETPAGESIZE, 0, 5);
  SetWindowTheme(careTrack, L"", L"");

  MkStatic(hwnd, IDC_CARE_OPTIONS_CAPTION, L"WHEN IDLE", pad, cy, contentW, 16, g_fontSection);
  MkButton(hwnd, IDC_CARE_CHROME, L"Dim icons, tray & clock (experimental)", pad, cy, contentW,
           40, BS_OWNERDRAW);
  MkButton(hwnd, IDC_CARE_NUDGE, L"Micro-nudge clock (experimental)", pad, cy, contentW, 40,
           BS_OWNERDRAW);
  MkButton(hwnd, IDC_CARE_NIGHT, L"Night boost (8 PM - 7 AM)", pad, cy, contentW, 40,
           BS_OWNERDRAW);
  MkButton(hwnd, IDC_CARE_PAUSE_LOCK, L"Pause themes while locked", pad, cy, contentW, 40,
           BS_OWNERDRAW);
  MkButton(hwnd, IDC_CARE_PAUSE_FS, L"Pause themes in fullscreen", pad, cy, contentW, 40,
           BS_OWNERDRAW);

  MkButton(hwnd, IDC_CARE_PRESET, L"OLED everyday preset", pad, cy, contentW, 40, BS_OWNERDRAW);
  MkStatic(hwnd, IDC_CARE_HINT,
           L"Applies only to monitors left on in the Monitors tab.", pad, cy, contentW, 20,
           g_fontUi);

  // --- Monitors tab ---
  const int my = y;
  MkStatic(hwnd, IDC_MONITORS_CAPTION, L"ACTIVE DISPLAYS", pad, my, contentW, 16,
           g_fontSection);
  for (int i = 0; i < static_cast<int>(kMaxMonitorSlots); ++i) {
    MkButton(hwnd, IDC_MONITOR_BASE + i, L"", pad, my + 28, contentW, 40, BS_OWNERDRAW);
  }
  MkStatic(hwnd, IDC_MONITORS_HINT, L"", pad, kClientH - 96, contentW, 32, g_fontUi);

  // --- First-run Welcome (no tab button; tab row hidden while shown) ---
  MkStatic(hwnd, IDC_WELCOME_INTRO, L"", pad, 70, contentW, 64, g_fontUi);
  MkStatic(hwnd, IDC_WELCOME_CAPTION, L"WHICH DISPLAYS ARE OLED?", pad, 150, contentW, 16,
           g_fontSection);
  MkStatic(hwnd, IDC_WELCOME_HINT,
           L"Done saves this choice. Customize opens the full Settings (themes, "
           L"intensity, OLED Care); change displays anytime in Settings › Monitors.",
           pad, 0, contentW, 44, g_fontUi);

  // --- System tab ---
  const int sy = y;
  MkStatic(hwnd, IDC_GENERAL_CAPTION, L"GENERAL", pad, sy, contentW, 16, g_fontSection);
  int syy = sy + 28;
  MkButton(hwnd, IDC_AUTOSTART, L"Start with Windows", pad, syy, contentW, 36, BS_OWNERDRAW);
  syy += 36 + 24;
  MkStatic(hwnd, IDC_RENDER_CAPTION, L"RENDERING", pad, syy, contentW, 16, g_fontSection);
  syy += 28;
  for (int i = 0; i < 3; ++i) {
    MkButton(hwnd, IDC_RENDER_BASE + i, RenderModeName(RenderModeFromIndex(i)), pad, syy,
             contentW, 34, BS_OWNERDRAW);
    syy += 34 + 8;
  }
  syy += 16;
  MkStatic(hwnd, IDC_UPDATES_CAPTION, L"UPDATES", pad, syy, contentW, 16, g_fontSection);
  syy += 28;
  MkStatic(hwnd, IDC_VERSION, L"Installed version", pad, syy, contentW, 18, g_fontUi);
  syy += 24;
  MkStatic(hwnd, IDC_UPDATE_STATUS, L"", pad, syy, contentW, 18, g_fontSection);
  syy += 28;
  MkButton(hwnd, IDC_CHECK_UPDATES, L"Check for updates", pad, syy, 180, 34, BS_OWNERDRAW);
  MkButton(hwnd, IDC_UPDATE_BTN, L"Update now", pad + contentW - 140, syy, 140, 34,
           BS_OWNERDRAW);
  syy += 34 + 24;
  MkStatic(hwnd, IDC_DIAG_CAPTION, L"TROUBLESHOOTING", pad, syy, contentW, 16, g_fontSection);
  syy += 28;
  MkButton(hwnd, IDC_COPY_DIAG, L"Copy diagnostics", pad, syy, 180, 34, BS_OWNERDRAW);
  MkStatic(hwnd, IDC_DIAG_STATUS,
           L"Version, Windows build, displays, settings and recent logs - paste into a GitHub issue.",
           pad + 196, syy, contentW - 196, 36, g_fontUi);

  const int footerY = kClientH - 52;
  MkButton(hwnd, IDC_CLOSE, L"Close", pad, footerY, 110, 34, BS_OWNERDRAW);
  MkButton(hwnd, IDC_APPLY, L"Apply", pad + contentW - 130, footerY, 130, 34, BS_OWNERDRAW);

  SyncDraftFromSettings();
  LoadControlsFromDraft(hwnd);
  ShowTab(hwnd, 0);
}

void FocusUpdatesSection(HWND hwnd) {
  ShowTab(hwnd, 3);
  HWND updateBtn = GetDlgItem(hwnd, IDC_UPDATE_BTN);
  if (IsWindowEnabled(updateBtn)) {
    SetFocus(updateBtn);
  } else {
    SetFocus(GetDlgItem(hwnd, IDC_CHECK_UPDATES));
  }
}

LRESULT CALLBACK SettingsWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE:
      EnableDarkTitleBar(hwnd);
      BuildChildren(hwnd);
      return 0;
    case WM_DISPLAYCHANGE:
      RefreshMonitorList();
      SyncDraftFromScope();
      LoadControlsFromDraft(hwnd);
      ShowTab(hwnd, g_activeTab);
      return 0;
    case WM_TIMER:
      if (wParam == ID_PREVIEW_TIMER && g_activeTab == 0) {
        g_previewTime += kPreviewIntervalMs / 1000.f;
        RefreshPreview(hwnd);
      }
      return 0;
    case WM_ERASEBKGND: {
      RECT rc{};
      GetClientRect(hwnd, &rc);
      FillRect(reinterpret_cast<HDC>(wParam), &rc, g_brScreen);
      return 1;
    }
    case WM_CTLCOLORSTATIC: {
      HDC hdc = reinterpret_cast<HDC>(wParam);
      SetBkColor(hdc, kScreen);
      const int id = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
      if (id == IDC_SUBTITLE || id == IDC_APPEARANCE_CAPTION || id == IDC_UPDATES_CAPTION ||
          id == IDC_INTENSITY_CAPTION || id == IDC_VERSION || id == IDC_PREVIEW_CAPTION ||
          id == IDC_PREVIEW_HINT || id == IDC_GENERAL_CAPTION || id == IDC_RENDER_CAPTION ||
          id == IDC_SCOPE_CAPTION || id == IDC_MONITORS_CAPTION || id == IDC_MONITORS_HINT ||
          id == IDC_CARE_SUBTITLE || id == IDC_CARE_IDLE_CAPTION ||
          id == IDC_CARE_FILL_CAPTION || id == IDC_CARE_OPTIONS_CAPTION || id == IDC_CARE_HINT ||
          id == IDC_WELCOME_CAPTION || id == IDC_WELCOME_HINT || id == IDC_DIAG_CAPTION ||
          id == IDC_DIAG_STATUS) {
        SetTextColor(hdc, kSoft);
      } else if (id == IDC_UPDATE_STATUS && HasPendingUpdate()) {
        SetTextColor(hdc, kEmber);
      } else {
        SetTextColor(hdc, kInk);
      }
      return reinterpret_cast<LRESULT>(g_brScreen);
    }
    case WM_DRAWITEM: {
      auto* dis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
      if (dis->CtlID == IDC_TAB_TASKBAR) {
        DrawTabButton(dis, L"Taskbar", g_activeTab == 0);
        return TRUE;
      }
      if (dis->CtlID == IDC_TAB_CARE) {
        DrawTabButton(dis, L"OLED Care", g_activeTab == 1);
        return TRUE;
      }
      if (dis->CtlID == IDC_TAB_MONITORS) {
        DrawTabButton(dis, L"Monitors", g_activeTab == 2);
        return TRUE;
      }
      if (dis->CtlID == IDC_TAB_SYSTEM) {
        DrawTabButton(dis, L"System", g_activeTab == 3);
        return TRUE;
      }
      if (dis->CtlID == IDC_PREVIEW) {
        DrawPreview(dis);
        return TRUE;
      }
      if (dis->CtlID >= IDC_SCOPE_BASE && dis->CtlID < IDC_SCOPE_BASE + kMaxScopeChips) {
        const int chip = dis->CtlID - IDC_SCOPE_BASE;
        const bool selected =
            (chip == 0 && g_scopeIndex < 0) || (chip > 0 && g_scopeIndex == chip - 1);
        bool dimmed = false;
        wchar_t label[16]{};
        if (chip == 0) {
          wcsncpy_s(label, L"All", _TRUNCATE);
        } else {
          swprintf_s(label, L"%d", chip);
          const int mi = chip - 1;
          if (mi < static_cast<int>(g_draftMonitorActive.size())) {
            dimmed = !g_draftMonitorActive[mi];
          }
        }
        DrawChip(dis, label, selected, dimmed);
        return TRUE;
      }
      if (dis->CtlID >= IDC_MONITOR_BASE &&
          dis->CtlID < IDC_MONITOR_BASE + static_cast<int>(kMaxMonitorSlots)) {
        DrawMonitorRow(dis, dis->CtlID - IDC_MONITOR_BASE);
        return TRUE;
      }
      if (dis->CtlID >= IDC_THEME_BASE &&
          dis->CtlID < IDC_THEME_BASE + static_cast<int>(ThemeId::Count)) {
        DrawThemeRow(dis, dis->CtlID - IDC_THEME_BASE);
        return TRUE;
      }
      if (dis->CtlID == IDC_AUTOSTART) {
        DrawCheckRow(dis, L"Start with Windows", g_draftAutostart);
        return TRUE;
      }
      if (dis->CtlID == IDC_CARE_CHROME) {
        DrawCheckRow(dis, L"Dim icons, tray & clock (experimental)", g_draftCareChromeDim);
        return TRUE;
      }
      if (dis->CtlID == IDC_CARE_NUDGE) {
        DrawCheckRow(dis, L"Micro-nudge clock (experimental)", g_draftCareClockNudge);
        return TRUE;
      }
      if (dis->CtlID == IDC_CARE_NIGHT) {
        DrawCheckRow(dis, L"Night boost (8 PM - 7 AM)", g_draftCareNightBoost);
        return TRUE;
      }
      if (dis->CtlID == IDC_CARE_PAUSE_LOCK) {
        DrawCheckRow(dis, L"Pause themes while locked", g_draftPauseWhenLocked);
        return TRUE;
      }
      if (dis->CtlID == IDC_CARE_PAUSE_FS) {
        DrawCheckRow(dis, L"Pause themes in fullscreen", g_draftPauseWhenFullscreen);
        return TRUE;
      }
      if (dis->CtlID >= IDC_CARE_IDLE_BASE &&
          dis->CtlID < IDC_CARE_IDLE_BASE + kCareIdleChipCount) {
        const int chip = dis->CtlID - IDC_CARE_IDLE_BASE;
        const bool selected = g_draftCareIdleMinutes == kOledCareIdleChoices[chip];
        DrawChip(dis, CareIdleChipLabel(chip), selected, false);
        return TRUE;
      }
      if (dis->CtlID >= IDC_RENDER_BASE && dis->CtlID < IDC_RENDER_BASE + 3) {
        DrawRenderRow(dis, dis->CtlID - IDC_RENDER_BASE);
        return TRUE;
      }
      if (dis->CtlID == IDC_APPLY || dis->CtlID == IDC_UPDATE_BTN ||
          dis->CtlID == IDC_CARE_PRESET) {
        const wchar_t* label = (g_activeTab == kTabWelcome) ? L"Done" : L"Apply";
        if (dis->CtlID == IDC_UPDATE_BTN) label = L"Update now";
        if (dis->CtlID == IDC_CARE_PRESET) label = L"OLED everyday preset";
        DrawPrimaryButton(dis, label, IsWindowEnabled(dis->hwndItem) != FALSE);
        return TRUE;
      }
      if (dis->CtlID == IDC_CLOSE || dis->CtlID == IDC_CHECK_UPDATES ||
          dis->CtlID == IDC_COPY_DIAG) {
        const wchar_t* label = (g_activeTab == kTabWelcome) ? L"Customize" : L"Close";
        if (dis->CtlID == IDC_CHECK_UPDATES) label = L"Check for updates";
        if (dis->CtlID == IDC_COPY_DIAG) label = L"Copy diagnostics";
        DrawGhostButton(dis, label, IsWindowEnabled(dis->hwndItem) != FALSE);
        return TRUE;
      }
      break;
    }
    case WM_HSCROLL: {
      const int trackId = GetDlgCtrlID(reinterpret_cast<HWND>(lParam));
      if (trackId == IDC_INTENSITY) {
        g_draftIntensity = static_cast<int>(
            SendDlgItemMessageW(hwnd, IDC_INTENSITY, TBM_GETPOS, 0, 0));
        g_draftIntensity = ClampPercent(g_draftIntensity);
        UpdateIntensityValue(hwnd);
        RefreshPreview(hwnd);
      } else if (trackId == IDC_CARE_FILL) {
        g_draftCareFillIntensity = static_cast<int>(
            SendDlgItemMessageW(hwnd, IDC_CARE_FILL, TBM_GETPOS, 0, 0));
        g_draftCareFillIntensity = ClampPercent(g_draftCareFillIntensity);
        UpdateCareFillValue(hwnd);
      }
      return 0;
    }
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == IDC_TAB_TASKBAR) {
        ShowTab(hwnd, 0);
        return 0;
      }
      if (id == IDC_TAB_CARE) {
        ShowTab(hwnd, 1);
        return 0;
      }
      if (id == IDC_TAB_MONITORS) {
        ShowTab(hwnd, 2);
        return 0;
      }
      if (id == IDC_TAB_SYSTEM) {
        ShowTab(hwnd, 3);
        return 0;
      }
      if (id >= IDC_SCOPE_BASE && id < IDC_SCOPE_BASE + kMaxScopeChips) {
        const int chip = id - IDC_SCOPE_BASE;
        g_scopeIndex = (chip == 0) ? -1 : chip - 1;
        SyncDraftFromScope();
        LoadControlsFromDraft(hwnd);
        return 0;
      }
      if (id >= IDC_MONITOR_BASE &&
          id < IDC_MONITOR_BASE + static_cast<int>(kMaxMonitorSlots)) {
        const int mi = id - IDC_MONITOR_BASE;
        if (mi >= 0 && mi < static_cast<int>(g_draftMonitorActive.size())) {
          g_draftMonitorActive[mi] = !g_draftMonitorActive[mi];
          InvalidateRect(GetDlgItem(hwnd, id), nullptr, TRUE);
          InvalidateScopeChips(hwnd);
          UpdateMonitorsHint(hwnd);
        }
        return 0;
      }
      if (id >= IDC_THEME_BASE &&
          id < IDC_THEME_BASE + static_cast<int>(ThemeId::Count)) {
        g_draftTheme = ThemeFromIndex(id - IDC_THEME_BASE);
        g_previewTime = ThemeBaseTime(g_draftTheme);
        InvalidateThemeRows(hwnd);
        RefreshPreview(hwnd);
        return 0;
      }
      if (id == IDC_AUTOSTART) {
        g_draftAutostart = !g_draftAutostart;
        CheckDlgButton(hwnd, IDC_AUTOSTART,
                       g_draftAutostart ? BST_CHECKED : BST_UNCHECKED);
        InvalidateRect(GetDlgItem(hwnd, IDC_AUTOSTART), nullptr, TRUE);
        return 0;
      }
      if (id >= IDC_CARE_IDLE_BASE && id < IDC_CARE_IDLE_BASE + kCareIdleChipCount) {
        g_draftCareIdleMinutes = kOledCareIdleChoices[id - IDC_CARE_IDLE_BASE];
        InvalidateCareIdleChips(hwnd);
        return 0;
      }
      if (id == IDC_CARE_CHROME) {
        g_draftCareChromeDim = !g_draftCareChromeDim;
        InvalidateRect(GetDlgItem(hwnd, IDC_CARE_CHROME), nullptr, TRUE);
        return 0;
      }
      if (id == IDC_CARE_NUDGE) {
        g_draftCareClockNudge = !g_draftCareClockNudge;
        InvalidateRect(GetDlgItem(hwnd, IDC_CARE_NUDGE), nullptr, TRUE);
        return 0;
      }
      if (id == IDC_CARE_NIGHT) {
        g_draftCareNightBoost = !g_draftCareNightBoost;
        InvalidateRect(GetDlgItem(hwnd, IDC_CARE_NIGHT), nullptr, TRUE);
        return 0;
      }
      if (id == IDC_CARE_PAUSE_LOCK) {
        g_draftPauseWhenLocked = !g_draftPauseWhenLocked;
        InvalidateRect(GetDlgItem(hwnd, IDC_CARE_PAUSE_LOCK), nullptr, TRUE);
        return 0;
      }
      if (id == IDC_CARE_PAUSE_FS) {
        g_draftPauseWhenFullscreen = !g_draftPauseWhenFullscreen;
        InvalidateRect(GetDlgItem(hwnd, IDC_CARE_PAUSE_FS), nullptr, TRUE);
        return 0;
      }
      if (id == IDC_CARE_PRESET) {
        ApplyOledEverydayPreset(hwnd);
        return 0;
      }
      if (id >= IDC_RENDER_BASE && id < IDC_RENDER_BASE + 3) {
        g_draftRenderMode = RenderModeFromIndex(id - IDC_RENDER_BASE);
        InvalidateRenderRows(hwnd);
        return 0;
      }
      if (id == IDC_APPLY) {
        ApplyDraft(hwnd);
        // Welcome "Done": save the display choice and get out of the way.
        if (g_activeTab == kTabWelcome) DestroyWindow(hwnd);
        return 0;
      }
      if (id == IDC_CLOSE && g_activeTab == kTabWelcome) {
        // "Customize": keep the display choice, continue in the full Settings.
        ApplyDraft(hwnd);
        ShowTab(hwnd, 0);
        return 0;
      }
      if (id == IDC_CLOSE || id == IDCANCEL) {
        DestroyWindow(hwnd);
        return 0;
      }
      if (id == IDC_CHECK_UPDATES) {
        if (g_host.onRequestUpdateCheck) g_host.onRequestUpdateCheck();
        RefreshUpdateControls(hwnd);
        return 0;
      }
      if (id == IDC_UPDATE_BTN) {
        if (g_host.onOpenUpdate) g_host.onOpenUpdate();
        return 0;
      }
      if (id == IDC_COPY_DIAG) {
        const bool ok = g_host.onCopyDiagnostics && g_host.onCopyDiagnostics(hwnd);
        SetWindowTextW(GetDlgItem(hwnd, IDC_DIAG_STATUS),
                       ok ? L"Copied to the clipboard. Paste it into a GitHub issue "
                            L"(your user folder is shown as %USERPROFILE%)."
                          : L"Could not open the clipboard - try again.");
        return 0;
      }
      return 0;
    }
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return 0;
    case WM_DESTROY:
      KillTimer(hwnd, ID_PREVIEW_TIMER);
      if (g_previewBmp) {
        DeleteObject(g_previewBmp);
        g_previewBmp = nullptr;
        g_previewBits = nullptr;
        g_previewBmpW = g_previewBmpH = 0;
      }
      g_settingsHwnd = nullptr;
      return 0;
    default:
      return DefWindowProcW(hwnd, msg, wParam, lParam);
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

bool EnsureClass(HINSTANCE hi) {
  if (g_classRegistered) return true;
  EnsureGdi();
  WNDCLASSEXW wc{sizeof(wc)};
  wc.style = CS_HREDRAW | CS_VREDRAW;
  wc.lpfnWndProc = SettingsWndProc;
  wc.hInstance = hi;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = g_brScreen;
  wc.hIcon = LoadIconW(hi, MAKEINTRESOURCEW(IDI_APPICON));
  wc.lpszClassName = kClassName;
  g_classRegistered = RegisterClassExW(&wc) != 0;
  return g_classRegistered;
}

SIZE WindowSizeForClient(DWORD style, DWORD ex, int clientW, int clientH) {
  RECT rc{0, 0, clientW, clientH};
  AdjustWindowRectEx(&rc, style, FALSE, ex);
  return {rc.right - rc.left, rc.bottom - rc.top};
}

}  // namespace

void SettingsWindowBind(const SettingsWindowHost& host) { g_host = host; }

bool SettingsWindowIsOpen() { return g_settingsHwnd != nullptr; }

HWND SettingsWindowHwnd() { return g_settingsHwnd; }

void SettingsWindowRefreshUpdateUi() {
  if (g_settingsHwnd) RefreshUpdateControls(g_settingsHwnd);
}

void SettingsWindowReloadAppearance() {
  if (!g_settingsHwnd || !g_host.settings) return;
  // Keep other unsaved drafts (Care, System); only the look changed under us.
  if (g_scopeIndex >= 0 && g_scopeIndex < static_cast<int>(g_monitors.size())) {
    LoadAppearanceDraft(
        ResolveMonitorAppearance(*g_host.settings, g_monitors[g_scopeIndex].deviceName));
  } else {
    MonitorAppearance a;
    a.theme = g_host.settings->theme;
    a.intensity = g_host.settings->intensity;
    LoadAppearanceDraft(a);
  }
  SendDlgItemMessageW(g_settingsHwnd, IDC_INTENSITY, TBM_SETPOS, TRUE,
                      static_cast<LPARAM>(g_draftIntensity));
  UpdateIntensityValue(g_settingsHwnd);
  InvalidateThemeRows(g_settingsHwnd);
  RefreshPreview(g_settingsHwnd);
  if (g_activeTab == kTabWelcome) UpdateWelcomeIntro(g_settingsHwnd);
}

void SettingsWindowRefreshPauseUi() {
  if (g_settingsHwnd) RefreshPauseStatus(g_settingsHwnd);
}

void SettingsWindowShow(HWND owner, HINSTANCE instance, SettingsFocus focus) {
  INITCOMMONCONTROLSEX icc{sizeof(icc),
                           ICC_BAR_CLASSES | ICC_STANDARD_CLASSES | ICC_TAB_CLASSES};
  InitCommonControlsEx(&icc);

  if (!EnsureClass(instance)) return;

  if (g_settingsHwnd) {
    ShowWindow(g_settingsHwnd, SW_SHOW);
    SetForegroundWindow(g_settingsHwnd);
    SyncDraftFromSettings();
    LoadControlsFromDraft(g_settingsHwnd);
    if (focus == SettingsFocus::Updates) {
      FocusUpdatesSection(g_settingsHwnd);
    } else if (focus == SettingsFocus::Welcome) {
      ShowTab(g_settingsHwnd, kTabWelcome);
    } else {
      ShowTab(g_settingsHwnd, 0);
    }
    return;
  }

  const DWORD style =
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
  const DWORD ex = WS_EX_APPWINDOW | WS_EX_COMPOSITED;
  const SIZE win = WindowSizeForClient(style, ex, kClientW, kClientH);

  RECT work{};
  SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
  const int x = work.left + ((work.right - work.left) - win.cx) / 2;
  const int y = work.top + ((work.bottom - work.top) - win.cy) / 3;

  g_settingsHwnd =
      CreateWindowExW(ex, kClassName, L"Phostide", style, x, y, win.cx, win.cy, owner,
                      nullptr, instance, nullptr);
  if (!g_settingsHwnd) return;

  ShowWindow(g_settingsHwnd, SW_SHOW);
  UpdateWindow(g_settingsHwnd);
  SetForegroundWindow(g_settingsHwnd);
  if (focus == SettingsFocus::Updates) FocusUpdatesSection(g_settingsHwnd);
  if (focus == SettingsFocus::Welcome) ShowTab(g_settingsHwnd, kTabWelcome);
}

}  // namespace phostide
