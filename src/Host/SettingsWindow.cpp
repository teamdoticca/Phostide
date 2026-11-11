#include "SettingsWindow.h"
#include "resource.h"
#include "../ExplorerTAP/PixelThemes.h"

#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>

#include <algorithm>
#include <string>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

namespace phostide {
namespace {

constexpr wchar_t kClassName[] = L"PhostideSettings";

constexpr int kClientW = 560;
constexpr int kClientH = 700;
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
constexpr int IDC_TAB_SYSTEM = 1511;
constexpr int IDC_GENERAL_CAPTION = 1512;
constexpr int IDC_RENDER_CAPTION = 1513;
constexpr int IDC_RENDER_BASE = 1520;  // +0 Auto, +1 GPU, +2 CPU

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
int g_activeTab = 0;  // 0 = Taskbar, 1 = System
float g_previewTime = 3.2f;
int g_previewW = 480;
HBITMAP g_previewBmp = nullptr;
void* g_previewBits = nullptr;
int g_previewBmpW = 0;
int g_previewBmpH = 0;

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

void SyncDraftFromSettings() {
  if (!g_host.settings) return;
  g_draftTheme = g_host.settings->theme;
  g_draftIntensity = g_host.settings->intensity;
  g_draftAutostart = g_host.settings->startWithWindows;
  g_draftRenderMode = g_host.settings->renderMode;
  g_previewTime = ThemeBaseTime(g_draftTheme);
}

void InvalidateRenderRows(HWND hwnd) {
  for (int i = 0; i < 3; ++i) {
    InvalidateRect(GetDlgItem(hwnd, IDC_RENDER_BASE + i), nullptr, TRUE);
  }
}

void UpdateIntensityValue(HWND hwnd) {
  wchar_t buf[32];
  swprintf_s(buf, L"%d%%", g_draftIntensity);
  SetWindowTextW(GetDlgItem(hwnd, IDC_INTENSITY_VALUE), buf);
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

  // Soft fake tray icons so it reads as a taskbar, not a bare strip.
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
  // No erase - avoids black flash between frames.
  InvalidateRect(GetDlgItem(hwnd, IDC_PREVIEW), nullptr, FALSE);
}

void RefreshUpdateControls(HWND hwnd) {
  if (!hwnd || !g_host.settings) return;

  wchar_t version[80];
  swprintf_s(version, L"Installed version  v%.*s", static_cast<int>(kAppVersion.size()),
             kAppVersion.data());
  SetWindowTextW(GetDlgItem(hwnd, IDC_VERSION), version);

  const bool checking = g_host.updateCheckRunning && *g_host.updateCheckRunning;
  wchar_t status[128];
  if (checking) {
    wcsncpy_s(status, L"Checking for updates...", _TRUNCATE);
  } else if (g_host.updateStatusText && !g_host.updateStatusText->empty()) {
    wcsncpy_s(status, g_host.updateStatusText->c_str(), _TRUNCATE);
  } else if (HasPendingUpdate()) {
    swprintf_s(status, L"Update available  %s", g_host.settings->pendingUpdateTag.c_str());
  } else {
    wcsncpy_s(status, L"You're up to date", _TRUNCATE);
  }
  SetWindowTextW(GetDlgItem(hwnd, IDC_UPDATE_STATUS), status);

  EnableWindow(GetDlgItem(hwnd, IDC_CHECK_UPDATES), checking ? FALSE : TRUE);
  EnableWindow(GetDlgItem(hwnd, IDC_UPDATE_BTN),
               (!checking && HasPendingUpdate()) ? TRUE : FALSE);
  InvalidateRect(GetDlgItem(hwnd, IDC_UPDATE_BTN), nullptr, TRUE);
  InvalidateRect(GetDlgItem(hwnd, IDC_UPDATE_STATUS), nullptr, TRUE);
}

void LoadControlsFromDraft(HWND hwnd) {
  SendDlgItemMessageW(hwnd, IDC_INTENSITY, TBM_SETPOS, TRUE,
                      static_cast<LPARAM>(g_draftIntensity));
  UpdateIntensityValue(hwnd);
  CheckDlgButton(hwnd, IDC_AUTOSTART, g_draftAutostart ? BST_CHECKED : BST_UNCHECKED);
  InvalidateThemeRows(hwnd);
  InvalidateRenderRows(hwnd);
  RefreshUpdateControls(hwnd);
  RefreshPreview(hwnd);
}

void ReadDraftFromControls(HWND hwnd) {
  g_draftIntensity =
      static_cast<int>(SendDlgItemMessageW(hwnd, IDC_INTENSITY, TBM_GETPOS, 0, 0));
  g_draftIntensity = (std::max)(0, (std::min)(100, g_draftIntensity));
  // Autostart is BS_OWNERDRAW — g_draftAutostart is the source of truth (toggled on
  // click). Do not overwrite from IsDlgButtonChecked; that stays stale.
  (void)hwnd;
}

void ApplyDraft(HWND hwnd) {
  if (!g_host.settings || !g_host.onApplyAppearance) return;
  ReadDraftFromControls(hwnd);
  g_host.settings->theme = g_draftTheme;
  g_host.settings->renderMode = g_draftRenderMode;
  g_host.settings->intensity = g_draftIntensity;
  g_host.settings->startWithWindows = g_draftAutostart;
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

void FillRoundRect(HDC hdc, const RECT& rc, COLORREF fill, COLORREF border, int radius) {
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
  FillRoundRect(dis->hDC, dis->rcItem, fill, fill, 10);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, text);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontSection));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  SelectObject(dis->hDC, old);
}

void DrawGhostButton(DRAWITEMSTRUCT* dis, const wchar_t* label, bool enabled) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  FillRoundRect(dis->hDC, dis->rcItem, pressed ? RGB(40, 40, 44) : kPanel, kLine, 10);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, enabled ? kInk : kSoft);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontUi));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  SelectObject(dis->hDC, old);
}

void DrawTabButton(DRAWITEMSTRUCT* dis, const wchar_t* label, bool active) {
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = active ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = active ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10);
  SetBkMode(dis->hDC, TRANSPARENT);
  SetTextColor(dis->hDC, active ? kInk : kSoft);
  HFONT old = static_cast<HFONT>(SelectObject(dis->hDC, g_fontSection));
  DrawTextW(dis->hDC, label, -1, const_cast<RECT*>(&dis->rcItem),
            DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  SelectObject(dis->hDC, old);
}

void DrawThemeRow(DRAWITEMSTRUCT* dis, int themeIndex) {
  const bool selected = g_draftTheme == ThemeFromIndex(themeIndex);
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = selected ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = selected ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10);
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
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  SelectObject(dis->hDC, old);
}

void DrawRenderRow(DRAWITEMSTRUCT* dis, int modeIndex) {
  const RenderMode mode = RenderModeFromIndex(modeIndex);
  const bool selected = g_draftRenderMode == mode;
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const COLORREF fill = selected ? RGB(36, 28, 22) : (pressed ? RGB(34, 34, 36) : kPanel);
  const COLORREF border = selected ? kEmber : kLine;
  FillRoundRect(dis->hDC, dis->rcItem, fill, border, 10);
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
  DrawTextW(dis->hDC, RenderModeName(mode), -1, &textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  SelectObject(dis->hDC, old);
}

void DrawCheckRow(DRAWITEMSTRUCT* dis) {
  const bool checked = g_draftAutostart;
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  FillRoundRect(dis->hDC, dis->rcItem, pressed ? RGB(34, 34, 36) : kPanel, kLine, 10);

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
  DrawTextW(dis->hDC, L"Start with Windows", -1, &textRc,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  SelectObject(dis->hDC, old);
}

void DrawPreview(DRAWITEMSTRUCT* dis) {
  // Double-buffer the whole control so we never flash the void fill alone.
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

void ShowTab(HWND hwnd, int tab) {
  g_activeTab = tab;
  const BOOL taskbar = (tab == 0) ? TRUE : FALSE;
  const BOOL system = (tab == 1) ? TRUE : FALSE;

  const int taskbarIds[] = {
      IDC_SUBTITLE,       IDC_PREVIEW_CAPTION, IDC_PREVIEW,         IDC_PREVIEW_HINT,
      IDC_APPEARANCE_CAPTION, IDC_INTENSITY_CAPTION, IDC_INTENSITY_VALUE, IDC_INTENSITY,
  };
  for (int id : taskbarIds) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, taskbar ? SW_SHOW : SW_HIDE);
  }
  for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i) {
    HWND c = GetDlgItem(hwnd, IDC_THEME_BASE + i);
    if (c) ShowWindow(c, taskbar ? SW_SHOW : SW_HIDE);
  }

  const int systemIds[] = {
      IDC_GENERAL_CAPTION, IDC_AUTOSTART,      IDC_RENDER_CAPTION,  IDC_RENDER_BASE,
      IDC_RENDER_BASE + 1, IDC_RENDER_BASE + 2, IDC_UPDATES_CAPTION, IDC_VERSION,
      IDC_UPDATE_STATUS,  IDC_CHECK_UPDATES, IDC_UPDATE_BTN,
  };
  for (int id : systemIds) {
    HWND c = GetDlgItem(hwnd, id);
    if (c) ShowWindow(c, system ? SW_SHOW : SW_HIDE);
  }

  InvalidateRect(GetDlgItem(hwnd, IDC_TAB_TASKBAR), nullptr, FALSE);
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
  int y = 18;

  MkStatic(hwnd, IDC_TITLE, L"Settings", pad, y, contentW, 30, g_fontTitle);
  y += 36;

  const int tabW = (contentW - 10) / 2;
  MkButton(hwnd, IDC_TAB_TASKBAR, L"Taskbar", pad, y, tabW, 34, BS_OWNERDRAW);
  MkButton(hwnd, IDC_TAB_SYSTEM, L"System", pad + tabW + 10, y, tabW, 34, BS_OWNERDRAW);
  y += 48;

  // --- Taskbar tab ---
  MkStatic(hwnd, IDC_SUBTITLE, L"Preview the taskbar, then Apply when it looks right.", pad,
           y, contentW, 18, g_fontUi);
  int ty = y + 26;

  MkStatic(hwnd, IDC_PREVIEW_CAPTION, L"TASKBAR PREVIEW", pad, ty, contentW, 16,
           g_fontSection);
  ty += 20;
  CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_OWNERDRAW | SS_NOTIFY, pad,
                  ty, contentW, kPreviewH + 8, hwnd,
                  reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_PREVIEW)),
                  ParentInstance(hwnd), nullptr);
  ty += kPreviewH + 12;
  MkStatic(hwnd, IDC_PREVIEW_HINT,
           L"Live preview - changes apply only when you click Apply.", pad, ty, contentW, 22,
           g_fontUi);
  ty += 32;

  const int colGap = 18;
  const int leftW = (contentW - colGap) * 55 / 100;
  const int rightW = contentW - colGap - leftW;
  const int leftX = pad;
  const int rightX = pad + leftW + colGap;

  MkStatic(hwnd, IDC_APPEARANCE_CAPTION, L"THEME", leftX, ty, leftW, 16, g_fontSection);
  int ly = ty + 22;
  const int themeH = 34;
  const int themeGap = 6;
  for (int i = 0; i < static_cast<int>(ThemeId::Count); ++i) {
    MkButton(hwnd, IDC_THEME_BASE + i, ThemeName(ThemeFromIndex(i)), leftX, ly, leftW,
             themeH, BS_OWNERDRAW);
    ly += themeH + themeGap;
  }

  MkStatic(hwnd, IDC_INTENSITY_CAPTION, L"INTENSITY", rightX, ty, rightW - 70, 16,
           g_fontSection);
  MkStatic(hwnd, IDC_INTENSITY_VALUE, L"0%", rightX + rightW - 64, ty - 2, 64, 22, g_fontValue,
           SS_RIGHT);
  int ry = ty + 24;
  HWND track = CreateWindowExW(0, TRACKBAR_CLASSW, L"",
                               WS_CHILD | WS_VISIBLE | WS_TABSTOP | TBS_HORZ | TBS_NOTICKS,
                               rightX - 4, ry, rightW + 8, 28, hwnd,
                               reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INTENSITY)),
                               ParentInstance(hwnd), nullptr);
  SendMessageW(track, TBM_SETRANGEMIN, FALSE, 0);
  SendMessageW(track, TBM_SETRANGEMAX, FALSE, 100);
  SendMessageW(track, TBM_SETPAGESIZE, 0, 5);
  SetWindowTheme(track, L"", L"");

  // --- System tab (same page Y as taskbar body start) ---
  const int sy = y;
  MkStatic(hwnd, IDC_GENERAL_CAPTION, L"GENERAL", pad, sy, contentW, 16, g_fontSection);
  int syy = sy + 22;
  MkButton(hwnd, IDC_AUTOSTART, L"Start with Windows", pad, syy, contentW, 36, BS_OWNERDRAW);
  syy += 52;
  MkStatic(hwnd, IDC_RENDER_CAPTION, L"RENDERING", pad, syy, contentW, 16, g_fontSection);
  syy += 22;
  for (int i = 0; i < 3; ++i) {
    MkButton(hwnd, IDC_RENDER_BASE + i, RenderModeName(RenderModeFromIndex(i)), pad, syy,
             contentW, 32, BS_OWNERDRAW);
    syy += 38;
  }
  syy += 10;
  MkStatic(hwnd, IDC_UPDATES_CAPTION, L"UPDATES", pad, syy, contentW, 16, g_fontSection);
  syy += 22;
  MkStatic(hwnd, IDC_VERSION, L"Installed version", pad, syy, contentW, 18, g_fontUi);
  syy += 22;
  MkStatic(hwnd, IDC_UPDATE_STATUS, L"", pad, syy, contentW, 18, g_fontSection);
  syy += 28;
  MkButton(hwnd, IDC_CHECK_UPDATES, L"Check for updates", pad, syy, 180, 34, BS_OWNERDRAW);
  MkButton(hwnd, IDC_UPDATE_BTN, L"Update", pad + contentW - 120, syy, 120, 34, BS_OWNERDRAW);

  const int footerY = kClientH - 56;
  MkButton(hwnd, IDC_CLOSE, L"Close", pad, footerY, 110, 36, BS_OWNERDRAW);
  MkButton(hwnd, IDC_APPLY, L"Apply", pad + contentW - 130, footerY, 130, 36, BS_OWNERDRAW);

  SyncDraftFromSettings();
  LoadControlsFromDraft(hwnd);
  ShowTab(hwnd, 0);
}

void FocusUpdatesSection(HWND hwnd) {
  ShowTab(hwnd, 1);
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
          id == IDC_PREVIEW_HINT || id == IDC_GENERAL_CAPTION || id == IDC_RENDER_CAPTION) {
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
      if (dis->CtlID == IDC_TAB_SYSTEM) {
        DrawTabButton(dis, L"System", g_activeTab == 1);
        return TRUE;
      }
      if (dis->CtlID == IDC_PREVIEW) {
        DrawPreview(dis);
        return TRUE;
      }
      if (dis->CtlID >= IDC_THEME_BASE &&
          dis->CtlID < IDC_THEME_BASE + static_cast<int>(ThemeId::Count)) {
        DrawThemeRow(dis, dis->CtlID - IDC_THEME_BASE);
        return TRUE;
      }
      if (dis->CtlID == IDC_AUTOSTART) {
        DrawCheckRow(dis);
        return TRUE;
      }
      if (dis->CtlID >= IDC_RENDER_BASE && dis->CtlID < IDC_RENDER_BASE + 3) {
        DrawRenderRow(dis, dis->CtlID - IDC_RENDER_BASE);
        return TRUE;
      }
      if (dis->CtlID == IDC_APPLY || dis->CtlID == IDC_UPDATE_BTN) {
        DrawPrimaryButton(dis, dis->CtlID == IDC_APPLY ? L"Apply" : L"Update",
                          IsWindowEnabled(dis->hwndItem) != FALSE);
        return TRUE;
      }
      if (dis->CtlID == IDC_CLOSE || dis->CtlID == IDC_CHECK_UPDATES) {
        DrawGhostButton(dis,
                        dis->CtlID == IDC_CLOSE ? L"Close" : L"Check for updates",
                        IsWindowEnabled(dis->hwndItem) != FALSE);
        return TRUE;
      }
      break;
    }
    case WM_HSCROLL:
      if (GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == IDC_INTENSITY) {
        g_draftIntensity = static_cast<int>(
            SendDlgItemMessageW(hwnd, IDC_INTENSITY, TBM_GETPOS, 0, 0));
        g_draftIntensity = (std::max)(0, (std::min)(100, g_draftIntensity));
        UpdateIntensityValue(hwnd);
        RefreshPreview(hwnd);
      }
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id == IDC_TAB_TASKBAR) {
        ShowTab(hwnd, 0);
        return 0;
      }
      if (id == IDC_TAB_SYSTEM) {
        ShowTab(hwnd, 1);
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
      if (id >= IDC_RENDER_BASE && id < IDC_RENDER_BASE + 3) {
        g_draftRenderMode = RenderModeFromIndex(id - IDC_RENDER_BASE);
        InvalidateRenderRows(hwnd);
        return 0;
      }
      if (id == IDC_APPLY) {
        ApplyDraft(hwnd);
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
}

}  // namespace phostide
