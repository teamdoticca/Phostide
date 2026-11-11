// ThemeLab — live CPU + GPU theme preview without explorer / TAP.
// Build: msbuild Phostide.sln /t:ThemeLab /p:Configuration=Release /p:Platform=x64
// Run:   out\Release\ThemeLab.exe   (needs Themes.hlsl next to the exe for GPU)

#include "LabGpu.h"
#include "../ExplorerTAP/PixelThemes.h"

#include <commctrl.h>
#include <windowsx.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>

#include <windows.h>

#pragma comment(lib, "comctl32.lib")

namespace {

constexpr wchar_t kClassName[] = L"PhostideThemeLab";
constexpr int kClientW = 1100;
constexpr int kClientH = 520;
constexpr int kPreviewNativeH = 48;
constexpr int kPreviewZoomH = 96;
constexpr int kPad = 16;
constexpr UINT_PTR kTimerId = 1;
constexpr UINT kTimerMs = 50;
constexpr int kShaderPollEveryN = 20;  // ~1s at 50ms ticks

constexpr int IDC_THEME0 = 2000;
constexpr int IDC_INTENSITY = 2100;
constexpr int IDC_INTENSITY_LABEL = 2101;
constexpr int IDC_HINT = 2102;
constexpr int IDC_GPU_STATUS = 2103;

phostide::ThemeId g_theme = phostide::ThemeId::Space;
int g_intensityPct = 100;
float g_time = 6.0f;

int g_previewW = 1000;
HBITMAP g_dibCpu = nullptr;
void* g_bitsCpu = nullptr;
HBITMAP g_dibGpu = nullptr;
void* g_bitsGpu = nullptr;
int g_dibW = 0;
int g_dibH = 0;
bool g_gpuOk = false;
std::wstring g_gpuStatus = L"GPU: starting…";
int g_shaderPollTicks = 0;

phostide::LabGpuPreview g_gpu;

HWND g_themeBtns[static_cast<int>(phostide::ThemeId::Count)]{};
HWND g_track = nullptr;
HWND g_intensityLabel = nullptr;
HWND g_gpuStatusLabel = nullptr;

float ThemeBaseTime(phostide::ThemeId id) {
  switch (id) {
    case phostide::ThemeId::Rainbow:  return 3.2f;
    case phostide::ThemeId::SeaWaves: return 5.5f;
    case phostide::ThemeId::Weather:  return 1.5f;
    case phostide::ThemeId::Space:    return 18.0f;  // Act C — star rush (not nebula-only open)
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

void UpdateGpuStatusLabel() {
  if (!g_gpuStatusLabel) return;
  SetWindowTextW(g_gpuStatusLabel, g_gpuStatus.c_str());
}

ULONGLONG QueryWriteTime(const std::wstring& path) {
  if (path.empty()) return 0;
  WIN32_FILE_ATTRIBUTE_DATA fad{};
  if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fad)) return 0;
  ULARGE_INTEGER u{};
  u.LowPart = fad.ftLastWriteTime.dwLowDateTime;
  u.HighPart = fad.ftLastWriteTime.dwHighDateTime;
  return u.QuadPart;
}

bool TryReloadGpuShaders(const wchar_t* reason) {
  if (!g_gpu.ReloadShaders()) {
    g_gpuStatus = L"GPU reload failed — " +
                  (g_gpu.LastError().empty() ? L"compile error" : g_gpu.LastError());
    g_gpuOk = g_gpu.Ready();
    UpdateGpuStatusLabel();
    return false;
  }
  g_gpuOk = true;
  g_gpuStatus = std::wstring(L"GPU reloaded (") + reason + L")";
  UpdateGpuStatusLabel();
  return true;
}

void PollShaderReload() {
  if (!g_gpuOk) return;
  const std::wstring& path = g_gpu.ShaderPath();
  if (path.empty()) return;
  const ULONGLONG wt = QueryWriteTime(path);
  if (wt == 0 || wt == g_gpu.ShaderWriteTime()) return;
  TryReloadGpuShaders(L"auto");
}

void SyncThemeButtons(HWND /*hwnd*/) {
  for (int i = 0; i < static_cast<int>(phostide::ThemeId::Count); ++i) {
    if (!g_themeBtns[i]) continue;
    const BOOL check = (static_cast<int>(g_theme) == i) ? BST_CHECKED : BST_UNCHECKED;
    SendMessageW(g_themeBtns[i], BM_SETCHECK, check, 0);
  }
}

bool EnsureDib(HBITMAP& dib, void*& bits, int w, int h) {
  if (dib && g_dibW == w && g_dibH == h && bits) return true;
  // Caller updates shared size after both DIBs are rebuilt.
  if (dib) {
    DeleteObject(dib);
    dib = nullptr;
    bits = nullptr;
  }
  BITMAPINFO bmi{};
  bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bmi.bmiHeader.biWidth = w;
  bmi.bmiHeader.biHeight = -h;
  bmi.bmiHeader.biPlanes = 1;
  bmi.bmiHeader.biBitCount = 32;
  bmi.bmiHeader.biCompression = BI_RGB;
  dib = CreateDIBSection(nullptr, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
  return dib && bits;
}

bool EnsureDibs(int w, int h) {
  if (g_dibCpu && g_dibGpu && g_dibW == w && g_dibH == h && g_bitsCpu && g_bitsGpu) return true;
  if (!EnsureDib(g_dibCpu, g_bitsCpu, w, h)) return false;
  if (!EnsureDib(g_dibGpu, g_bitsGpu, w, h)) return false;
  g_dibW = w;
  g_dibH = h;
  return true;
}

void PremulToOpaqueOverDark(uint8_t* bgra, int w, int h, int stride) {
  // Themes.hlsl writes premultiplied BGRA. GDI BitBlt ignores alpha, so bake
  // onto a near-black backdrop matching the lab chrome.
  constexpr float kBg = 17.f / 255.f;
  for (int y = 0; y < h; ++y) {
    uint8_t* row = bgra + y * stride;
    for (int x = 0; x < w; ++x) {
      uint8_t* p = row + x * 4;
      const float a = p[3] / 255.f;
      const float inv = 1.f - a;
      auto ch = [&](uint8_t c) {
        return static_cast<uint8_t>(
            std::clamp(c / 255.f + kBg * inv, 0.f, 1.f) * 255.f + 0.5f);
      };
      p[0] = ch(p[0]);
      p[1] = ch(p[1]);
      p[2] = ch(p[2]);
      p[3] = 255;
    }
  }
}

void RenderFrame() {
  if (g_previewW < 64) g_previewW = 64;
  const int w = g_previewW;
  const int h = kPreviewNativeH;
  if (!EnsureDibs(w, h)) return;

  const int stride = w * 4;
  auto* cpu = static_cast<uint8_t*>(g_bitsCpu);
  std::memset(cpu, 0, static_cast<size_t>(stride) * static_cast<size_t>(h));

  phostide::PaintContext ctx;
  ctx.theme = g_theme;
  ctx.intensity = static_cast<float>(g_intensityPct) / 100.f;
  ctx.timeSeconds = g_time;
  ctx.edge = phostide::TaskbarEdge::Bottom;
  ctx.vertical = false;
  ctx.thickness = h;
  ctx.length = w;
  phostide::PaintThemePixels(ctx, cpu, w, h, stride);

  auto* gpu = static_cast<uint8_t*>(g_bitsGpu);
  std::memset(gpu, 0, static_cast<size_t>(stride) * static_cast<size_t>(h));
  if (!g_gpuOk) {
    g_gpuOk = g_gpu.Ensure();
    if (g_gpuOk) {
      g_gpuStatus = L"GPU: D3D11 Themes.hlsl";
    } else {
      g_gpuStatus = L"GPU: " + (g_gpu.LastError().empty() ? L"unavailable" : g_gpu.LastError());
    }
    UpdateGpuStatusLabel();
  }
  if (g_gpuOk) {
    if (g_gpu.Render(g_theme, ctx.intensity, g_time, phostide::TaskbarEdge::Bottom, w, h, gpu,
                     stride)) {
      PremulToOpaqueOverDark(gpu, w, h, stride);
    } else {
      g_gpuOk = false;
      g_gpuStatus = L"GPU render failed — " + g_gpu.LastError();
      UpdateGpuStatusLabel();
    }
  }
}

int StripStackTop() { return kPad; }

RECT PreviewInvalidateRect() {
  // CPU label+strip, GPU label+strip, zoom label+strip.
  const int top = StripStackTop();
  const int bottom = top + 22 + kPreviewNativeH + 22 + kPreviewNativeH + 22 + kPreviewZoomH + 8;
  const int right = kPad + g_previewW + 2;
  return RECT{kPad - 2, top, right, bottom};
}

void InvalidatePreview(HWND hwnd) {
  RECT r = PreviewInvalidateRect();
  InvalidateRect(hwnd, &r, FALSE);
}

void PaintPreview(HDC hdc) {
  const int stripX = kPad;
  const int cpuY = kPad + 22;
  const int gpuY = cpuY + kPreviewNativeH + 22;
  const int zoomY = gpuY + kPreviewNativeH + 22;
  const int stripW = g_previewW;
  const int nativeH = kPreviewNativeH;
  const int zoomH = kPreviewZoomH;

  RECT area = PreviewInvalidateRect();
  const int bufW = area.right - area.left;
  const int bufH = area.bottom - area.top;
  if (bufW <= 0 || bufH <= 0) return;

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
  TextOutW(mem, stripX + ox, kPad + 2 + oy, L"CPU 48px", 8);
  TextOutW(mem, stripX + ox, cpuY + nativeH + 4 + oy, L"GPU 48px  (Themes.hlsl)", 23);
  TextOutW(mem, stripX + ox, gpuY + nativeH + 4 + oy, L"GPU Zoom 2x", 11);

  auto frame = [&](int x, int y, int w, int h) {
    RECT r{x + ox - 1, y + oy - 1, x + ox + w + 1, y + oy + h + 1};
    HBRUSH br = CreateSolidBrush(RGB(55, 55, 58));
    FrameRect(mem, &r, br);
    DeleteObject(br);
  };
  frame(stripX, cpuY, stripW, nativeH);
  frame(stripX, gpuY, stripW, nativeH);
  frame(stripX, zoomY, stripW, zoomH);

  auto blit = [&](HBITMAP dib, int x, int y, int dw, int dh) {
    if (!dib) return;
    HDC src = CreateCompatibleDC(mem);
    HGDIOBJ old = SelectObject(src, dib);
    SetStretchBltMode(mem, COLORONCOLOR);
    StretchBlt(mem, x + ox, y + oy, dw, dh, src, 0, 0, g_dibW, g_dibH, SRCCOPY);
    SelectObject(src, old);
    DeleteDC(src);
  };
  blit(g_dibCpu, stripX, cpuY, stripW, nativeH);
  blit(g_dibGpu, stripX, gpuY, stripW, nativeH);
  blit(g_dibGpu, stripX, zoomY, stripW, zoomH);

  BitBlt(hdc, area.left, area.top, bufW, bufH, mem, 0, 0, SRCCOPY);

  SelectObject(mem, oldBmp);
  DeleteObject(back);
  DeleteDC(mem);
}

void PaintWindow(HWND hwnd) {
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd, &ps);

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

  const int btnY = PreviewInvalidateRect().bottom + 12;
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
  if (g_gpuStatusLabel) {
    SetWindowPos(g_gpuStatusLabel, nullptr, kPad + 490, trackY, 480, 22, SWP_NOZORDER | SWP_NOACTIVATE);
  }

  HWND hint = GetDlgItem(hwnd, IDC_HINT);
  if (hint) {
    SetWindowPos(hint, nullptr, kPad, trackY + 28, 780, 22, SWP_NOZORDER | SWP_NOACTIVATE);
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

      g_gpuStatusLabel =
          CreateWindowExW(0, L"STATIC", g_gpuStatus.c_str(), WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
                          hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_GPU_STATUS)),
                          reinterpret_cast<LPCREATESTRUCT>(lParam)->hInstance, nullptr);

      CreateWindowExW(0, L"STATIC",
                      L"Esc closes  ·  1-5 themes  ·  F5 reloads Themes.hlsl  ·  no Explorer restart needed",
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
        if (++g_shaderPollTicks >= kShaderPollEveryN) {
          g_shaderPollTicks = 0;
          PollShaderReload();
        }
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
      if (wParam == VK_F5) {
        TryReloadGpuShaders(L"F5");
        RenderFrame();
        InvalidatePreview(hwnd);
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
      if (g_dibCpu) {
        DeleteObject(g_dibCpu);
        g_dibCpu = nullptr;
        g_bitsCpu = nullptr;
      }
      if (g_dibGpu) {
        DeleteObject(g_dibGpu);
        g_dibGpu = nullptr;
        g_bitsGpu = nullptr;
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

  // Pin to the primary monitor — lab is often launched while Cursor sits on
  // another display, and CW_USEDEFAULT can park the window off-screen.
  const int posX = GetSystemMetrics(SM_XVIRTUALSCREEN) < 0 ? 80 : 80;
  const int posY = 80;
  HWND hwnd = CreateWindowExW(0, kClassName, L"Phostide ThemeLab (CPU + GPU)", style, posX, posY,
                              winW, winH, nullptr, nullptr, instance, nullptr);
  if (!hwnd) return 1;

  ShowWindow(hwnd, show);
  SetForegroundWindow(hwnd);
  UpdateWindow(hwnd);

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
    if (msg.message == WM_KEYDOWN &&
        (msg.wParam == VK_ESCAPE || msg.wParam == VK_F5 ||
         (msg.wParam >= '1' && msg.wParam <= '5'))) {
      SendMessageW(hwnd, WM_KEYDOWN, msg.wParam, msg.lParam);
      continue;
    }
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
  return static_cast<int>(msg.wParam);
}
