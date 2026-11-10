// ThemeLab — live PaintThemePixels preview without explorer / TAP.
// Build: msbuild Phostide.sln /t:ThemeLab /p:Configuration=Release /p:Platform=x64
// Run:   out\Release\ThemeLab.exe

#include "../ExplorerTAP/PixelThemes.h"

#include <commctrl.h>
#include <windowsx.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#pragma comment(lib, "comctl32.lib")

namespace {

constexpr wchar_t kClassName[] = L"PhostideThemeLab";
constexpr int kClientW = 1100;
constexpr int kClientH = 320;
constexpr int kPreviewNativeH = 48;
constexpr int kPreviewZoomH = 96;
constexpr int kPad = 16;
constexpr UINT_PTR kTimerId = 1;
constexpr UINT kTimerMs = 50;

constexpr int IDC_THEME0 = 2000;
constexpr int IDC_INTENSITY = 2100;
constexpr int IDC_INTENSITY_LABEL = 2101;
constexpr int IDC_HINT = 2102;

phostide::ThemeId g_theme = phostide::ThemeId::SeaWaves;
int g_intensityPct = 100;
float g_time = 5.5f;

int g_previewW = 1000;
HBITMAP g_dib = nullptr;
void* g_bits = nullptr;
int g_dibW = 0;
int g_dibH = 0;

HWND g_themeBtns[static_cast<int>(phostide::ThemeId::Count)]{};
HWND g_track = nullptr;
HWND g_intensityLabel = nullptr;

float ThemeBaseTime(phostide::ThemeId id) {
  switch (id) {
    case phostide::ThemeId::Rainbow:  return 3.2f;
    case phostide::ThemeId::SeaWaves: return 5.5f;
    case phostide::ThemeId::Weather:  return 1.5f;  // near first purple flash
    case phostide::ThemeId::Space:    return 6.0f;
    case phostide::ThemeId::Aurora:   return 4.5f;
    default:                          return 3.0f;
  }
}

void UpdateIntensityLabel() {
  if (!g_intensityLabel) return;
  wchar_t buf[48];
  swprintf_s(buf, L"Intensity  %d%%", g_intensityPct);
  SetWindowTextW(g_intensityLabel, buf);
}

void SyncThemeButtons(HWND /*hwnd*/) {
  for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
    if (!g_themeBtns[i]) continue;
    const BOOL check = (static_cast<int>(g_theme) == i) ? BST_CHECKED : BST_UNCHECKED;
    SendMessageW(g_themeBtns[i], BM_SETCHECK, check, 0);
  }
}

bool EnsureDib(int w, int h) {
  if (g_dib && g_dibW == w && g_dibH == h && g_bits) return true;
  if (g_dib) {
    DeleteObject(g_dib);
    g_dib = nullptr;
    g_bits = nullptr;
  }
  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h;  // top-down
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  g_dib = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &g_bits, nullptr, 0);
  if (!g_dib || !g_bits) {
    g_dib = nullptr;
    g_bits = nullptr;
    g_dibW = g_dibH = 0;
    return false;
  }
  g_dibW = w;
  g_dibH = h;
  return true;
}

void RenderFrame() {
  if (g_previewW < 64) g_previewW = 64;
  const int w = g_previewW;
  const int h = kPreviewNativeH;
  if (!EnsureDib(w, h)) return;

  auto* pixels = static_cast<uint8_t*>(g_bits);
  const int stride = w * 4;
  std::memset(pixels, 0, static_cast<size_t>(stride) * static_cast<size_t>(h));

  phostide::PaintContext ctx;
  ctx.theme = g_theme;
  ctx.intensity = static_cast<float>(g_intensityPct) / 100.f;
  ctx.timeSeconds = g_time;
  ctx.edge = phostide::TaskbarEdge::Bottom;
  ctx.vertical = false;
  ctx.thickness = h;
  ctx.length = w;
  phostide::PaintThemePixels(ctx, pixels, w, h, stride);
}

RECT PreviewInvalidateRect() {
  // Labels + both strips + 1px frames.
  const int top = kPad;
  const int bottom = kPad + 28 + kPreviewNativeH + 28 + kPreviewZoomH + 2;
  const int right = kPad + g_previewW + 2;
  return RECT{kPad - 2, top, right, bottom};
}

void InvalidatePreview(HWND hwnd) {
  RECT r = PreviewInvalidateRect();
  InvalidateRect(hwnd, &r, FALSE);
}

void PaintPreview(HDC hdc) {
  const int stripX = kPad;
  const int stripY = kPad + 28;
  const int stripW = g_previewW;
  const int nativeH = kPreviewNativeH;
  const int zoomY = stripY + nativeH + 28;
  const int zoomH = kPreviewZoomH;

  RECT area = PreviewInvalidateRect();
  const int bufW = area.right - area.left;
  const int bufH = area.bottom - area.top;
  if (bufW <= 0 || bufH <= 0) return;

  // Compose offscreen, then one BitBlt — no tear / no child flash.
  HDC mem = CreateCompatibleDC(hdc);
  HBITMAP back = CreateCompatibleBitmap(hdc, bufW, bufH);
  HGDIOBJ oldBmp = SelectObject(mem, back);

  HBRUSH bg = CreateSolidBrush(RGB(17, 17, 17));
  RECT local{0, 0, bufW, bufH};
  FillRect(mem, &local, bg);
  DeleteObject(bg);

  const int ox = -area.left;
  const int oy = -area.top;

  SetBkMode(mem, TRANSPARENT);
  SetTextColor(mem, RGB(150, 150, 155));
  TextOutW(mem, stripX + ox, kPad + 4 + oy, L"Native 48px", 11);
  TextOutW(mem, stripX + ox, stripY + nativeH + 10 + oy, L"Zoom 2x", 7);

  auto frame = [&](int x, int y, int w, int h) {
    RECT r{x + ox - 1, y + oy - 1, x + ox + w + 1, y + oy + h + 1};
    HBRUSH br = CreateSolidBrush(RGB(55, 55, 58));
    FrameRect(mem, &r, br);
    DeleteObject(br);
  };
  frame(stripX, stripY, stripW, nativeH);
  frame(stripX, zoomY, stripW, zoomH);

  if (g_dib && g_bits) {
    HDC src = CreateCompatibleDC(mem);
    HGDIOBJ old = SelectObject(src, g_dib);
    SetStretchBltMode(mem, COLORONCOLOR);
    BitBlt(mem, stripX + ox, stripY + oy, stripW, nativeH, src, 0, 0, SRCCOPY);
    StretchBlt(mem, stripX + ox, zoomY + oy, stripW, zoomH, src, 0, 0, g_dibW, g_dibH, SRCCOPY);
    SelectObject(src, old);
    DeleteDC(src);
  }

  BitBlt(hdc, area.left, area.top, bufW, bufH, mem, 0, 0, SRCCOPY);

  SelectObject(mem, oldBmp);
  DeleteObject(back);
  DeleteDC(mem);
}

void PaintWindow(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);

  // Fill only non-preview chrome if needed (first paint / resize). Children are clipped.
  RECT client{};
  GetClientRect(hwnd, &client);
  RECT preview = PreviewInvalidateRect();
  HRGN clientRgn = CreateRectRgnIndirect(&client);
  HRGN previewRgn = CreateRectRgnIndirect(&preview);
  CombineRgn(clientRgn, clientRgn, previewRgn, RGN_DIFF);
  HBRUSH bg = CreateSolidBrush(RGB(17, 17, 17));
  FillRgn(hdc, clientRgn, bg);
  DeleteObject(bg);
  DeleteObject(clientRgn);
  DeleteObject(previewRgn);

  PaintPreview(hdc);

  EndPaint(hwnd, &ps);
}

void LayoutControls(HWND hwnd) {
  RECT rc{};
  GetClientRect(hwnd, &rc);
  g_previewW = (std::max)(640, static_cast<int>(rc.right) - kPad * 2);

  const int btnY = kPad + 28 + kPreviewNativeH + 28 + kPreviewZoomH + 18;
  const int btnW = 118;
  const int btnH = 28;
  const int btnGap = 8;
  for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
    if (!g_themeBtns[i]) continue;
    SetWindowPos(g_themeBtns[i], nullptr, kPad + i * (btnW + btnGap), btnY, btnW, btnH,
                 SWP_NOZORDER | SWP_NOACTIVATE);
  }

  const int trackY = btnY + btnH + 14;
  if (g_intensityLabel) {
    SetWindowPos(g_intensityLabel, nullptr, kPad, trackY, 140, 22, SWP_NOZORDER | SWP_NOACTIVATE);
  }
  if (g_track) {
    SetWindowPos(g_track, nullptr, kPad + 150, trackY - 4, 320, 30, SWP_NOZORDER | SWP_NOACTIVATE);
  }

  HWND hint = GetDlgItem(hwnd, IDC_HINT);
  if (hint) {
    SetWindowPos(hint, nullptr, kPad + 490, trackY, 420, 22, SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
    case WM_CREATE: {
      INITCOMMONCONTROLSEX icc{sizeof(icc), ICC_BAR_CLASSES};
      InitCommonControlsEx(&icc);

      for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
        const auto id = phostide::ThemeFromIndex(i);
        DWORD style = WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON;
        if (i == 0) style |= WS_GROUP;
        g_themeBtns[i] = CreateWindowExW(
            0, L"BUTTON", phostide::ThemeName(id), style, 0, 0, 0, 0, hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_THEME0 + i)),
            reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);
      }

      g_intensityLabel =
          CreateWindowExW(0, L"STATIC", L"Intensity  100%", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                          hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INTENSITY_LABEL)),
                          reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);

      g_track = CreateWindowExW(
          0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ, 0, 0, 0, 0,
          hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_INTENSITY)),
          reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);
      SendMessageW(g_track, TBM_SETRANGEMIN, FALSE, 0);
      SendMessageW(g_track, TBM_SETRANGEMAX, FALSE, 100);
      SendMessageW(g_track, TBM_SETPOS, TRUE, g_intensityPct);
      SendMessageW(g_track, TBM_SETTICFREQ, 10, 0);

      CreateWindowExW(0, L"STATIC", L"Esc closes  ·  1-5 themes  ·  no explorer needed",
                      WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd,
                      reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_HINT)),
                      reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);

      g_time = ThemeBaseTime(g_theme);
      SyncThemeButtons(hwnd);
      UpdateIntensityLabel();
      LayoutControls(hwnd);
      RenderFrame();
      SetTimer(hwnd, kTimerId, kTimerMs, nullptr);
      return 0;
    }
    case WM_SIZE:
      LayoutControls(hwnd);
      RenderFrame();
      InvalidateRect(hwnd, nullptr, FALSE);
      return 0;
    case WM_TIMER:
      if (wParam == kTimerId) {
        g_time += kTimerMs / 1000.f;
        RenderFrame();
        InvalidatePreview(hwnd);
      }
      return 0;
    case WM_COMMAND: {
      const int id = LOWORD(wParam);
      if (id >= IDC_THEME0 && id < IDC_THEME0 + static_cast<int>(phostide::ThemeId::Count)) {
        g_theme = phostide::ThemeFromIndex(id - IDC_THEME0);
        g_time = ThemeBaseTime(g_theme);
        SyncThemeButtons(hwnd);
        RenderFrame();
        InvalidatePreview(hwnd);
      }
      return 0;
    }
    case WM_HSCROLL:
      if (reinterpret_cast<HWND>(lParam) == g_track ||
          GetDlgCtrlID(reinterpret_cast<HWND>(lParam)) == IDC_INTENSITY) {
        g_intensityPct = static_cast<int>(SendMessageW(g_track, TBM_GETPOS, 0, 0));
        UpdateIntensityLabel();
        RenderFrame();
        InvalidatePreview(hwnd);
      }
      return 0;
    case WM_KEYDOWN:
      if (wParam == VK_ESCAPE) {
        DestroyWindow(hwnd);
        return 0;
      }
      if (wParam >= '1' && wParam <= '5') {
        const int idx = static_cast<int>(wParam - '1');
        if (idx < static_cast<int>(phostide::ThemeId::Count)) {
          g_theme = phostide::ThemeFromIndex(idx);
          g_time = ThemeBaseTime(g_theme);
          SyncThemeButtons(hwnd);
          RenderFrame();
          InvalidatePreview(hwnd);
        }
        return 0;
      }
      break;
    case WM_PAINT:
      PaintWindow(hwnd);
      return 0;
    case WM_ERASEBKGND:
      return 1;
    case WM_DESTROY:
      KillTimer(hwnd, kTimerId);
      if (g_dib) {
        DeleteObject(g_dib);
        g_dib = nullptr;
        g_bits = nullptr;
      }
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

}  // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
  WNDCLASSEXW wc{sizeof(wc)};
  wc.lpfnWndProc = WndProc;
  wc.hInstance = instance;
  wc.lpszClassName = kClassName;
  wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
  wc.hbrBackground = nullptr;
  wc.style = CS_HREDRAW | CS_VREDRAW;
  if (!RegisterClassExW(&wc)) return 1;

  const DWORD style =
      WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_THICKFRAME | WS_CLIPCHILDREN;
  RECT wr{0, 0, kClientW, kClientH};
  AdjustWindowRectEx(&wr, style, FALSE, 0);
  const int winW = wr.right - wr.left;
  const int winH = wr.bottom - wr.top;

  HWND hwnd =
      CreateWindowExW(0, kClassName, L"Phostide ThemeLab", style, CW_USEDEFAULT, CW_USEDEFAULT,
                      winW, winH, nullptr, nullptr, instance, nullptr);
  if (!hwnd) return 1;

  ShowWindow(hwnd, show);
  UpdateWindow(hwnd);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (msg.message == WM_KEYDOWN &&
        (msg.wParam == VK_ESCAPE || (msg.wParam >= '1' && msg.wParam <= '5'))) {
      SendMessageW(hwnd, WM_KEYDOWN, msg.wParam, msg.lParam);
      continue;
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}
