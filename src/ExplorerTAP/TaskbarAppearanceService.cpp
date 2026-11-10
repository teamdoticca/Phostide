#include "TaskbarAppearanceService.h"

#include "PixelThemes.h"
#include "TaskbarGeometry.h"

#include <algorithm>
#include <cmath>

#include <wrl/client.h>
#include <robuffer.h>

#include <winrt/Windows.Storage.Streams.h>

using Microsoft::WRL::ComPtr;
namespace wux = winrt::Windows::UI::Xaml;
namespace wuxm = winrt::Windows::UI::Xaml::Media;
namespace wuxmi = winrt::Windows::UI::Xaml::Media::Imaging;
namespace wuxs = winrt::Windows::UI::Xaml::Shapes;
namespace wsys = winrt::Windows::System;

namespace phostide {
namespace {

void SetBoundEvent() {
  if (HANDLE ev = CreateEventW(nullptr, TRUE, FALSE, kTapBoundEvent.data())) {
    SetEvent(ev);
    CloseHandle(ev);
  }
}

uint8_t* BufferData(winrt::Windows::Storage::Streams::IBuffer const& buffer) {
  ComPtr<::Windows::Storage::Streams::IBufferByteAccess> access;
  const auto hr = reinterpret_cast<IUnknown*>(winrt::get_abi(buffer))
                      ->QueryInterface(IID_PPV_ARGS(&access));
  if (FAILED(hr) || !access) return nullptr;
  byte* data = nullptr;
  if (FAILED(access->Buffer(&data))) return nullptr;
  return reinterpret_cast<uint8_t*>(data);
}

void MeasureShape(wuxs::Shape const& shape, int& w, int& h) {
  w = 960;
  h = 48;
  try {
    const auto size = shape.RenderSize();
    w = static_cast<int>(std::lround(size.Width));
    h = static_cast<int>(std::lround(size.Height));
    if (auto fe = shape.try_as<wux::FrameworkElement>()) {
      const int aw = static_cast<int>(std::lround(fe.ActualWidth()));
      const int ah = static_cast<int>(std::lround(fe.ActualHeight()));
      // Win11 BackgroundFill often keeps a stub RenderSize (8×8) while Actual*
      // already reflects the laid-out bar. Prefer the larger reliable measure.
      if (aw > w) w = aw;
      if (ah > h) h = ah;
      try {
        const auto as = fe.ActualSize();
        const int asw = static_cast<int>(std::lround(as.x));
        const int ash = static_cast<int>(std::lround(as.y));
        if (asw > w) w = asw;
        if (ash > h) h = ash;
      } catch (...) {
      }
    }
    // On-screen bounds beat local stub sizes when the shell scales the fill.
    try {
      if (auto fe = shape.try_as<wux::FrameworkElement>()) {
        const float localW =
            (std::max)(1.f, static_cast<float>((std::max)(w, static_cast<int>(
                                                                  std::lround(size.Width)))));
        const float localH =
            (std::max)(1.f, static_cast<float>((std::max)(h, static_cast<int>(
                                                                  std::lround(size.Height)))));
        const auto tt = fe.TransformToVisual(nullptr);
        const auto rc = tt.TransformBounds(
            winrt::Windows::Foundation::Rect{0.f, 0.f, localW, localH});
        const int bw = static_cast<int>(std::lround(std::abs(rc.Width)));
        const int bh = static_cast<int>(std::lround(std::abs(rc.Height)));
        // Only trust transformed bounds when they look like a taskbar strip.
        if (bw >= 64 && bh >= 8 && (bw * bh) > (w * h)) {
          w = bw;
          h = bh;
        }
      }
    } catch (...) {
    }
  } catch (...) {
  }
  w = (std::max)(8, (std::min)(w, 7680));
  h = (std::max)(8, (std::min)(h, 7680));
}

const wchar_t* EdgeName(TaskbarEdge e) {
  switch (e) {
    case TaskbarEdge::Top: return L"Top";
    case TaskbarEdge::Left: return L"Left";
    case TaskbarEdge::Right: return L"Right";
    case TaskbarEdge::Bottom:
    default: return L"Bottom";
  }
}

void LogService(const wchar_t* msg) {
  wchar_t dir[MAX_PATH]{};
  GetTempPathW(MAX_PATH, dir);
  wcscat_s(dir, L"Phostide");
  CreateDirectoryW(dir, nullptr);
  wchar_t path[MAX_PATH]{};
  swprintf_s(path, L"%s\\tap.log", dir);
  HANDLE f = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return;
  SYSTEMTIME st{};
  GetLocalTime(&st);
  wchar_t line[512];
  swprintf_s(line, L"%02u:%02u:%02u %s\r\n", st.wHour, st.wMinute, st.wSecond, msg);
  DWORD written = 0;
  WriteFile(f, line, static_cast<DWORD>(wcslen(line) * sizeof(wchar_t)), &written, nullptr);
  CloseHandle(f);
}

}  // namespace

TaskbarAppearanceService& TaskbarAppearanceService::Instance() {
  static TaskbarAppearanceService instance;
  return instance;
}

void TaskbarAppearanceService::SignalBound() { SetBoundEvent(); }

void TaskbarAppearanceService::RegisterBackground(wuxs::Shape const& shape) {
  if (!shape) return;
  if (!dispatcher_) {
    dispatcher_ = wsys::DispatcherQueue::GetForCurrentThread();
  }

  TaskbarVisual* target = nullptr;
  {
    std::lock_guard lock(mutex_);
    for (auto& tv : taskbars_) {
      if (tv.background == shape) return;
    }
    TaskbarVisual tv;
    tv.background = shape;
    try {
      tv.originalFill = shape.Fill();
    } catch (...) {
    }
    taskbars_.push_back(std::move(tv));
    target = &taskbars_.back();
  }

  if (target && BindVisual(*target)) {
    try {
      target->background.SizeChanged([this, shape](auto&&, auto&&) {
        std::lock_guard lock(mutex_);
        for (auto& tv : taskbars_) {
          if (tv.background == shape && tv.bound) {
            RefreshGeomAndBitmap(tv);
          }
        }
      });
    } catch (...) {
    }
    boundCount_.fetch_add(1);
    SignalBound();
    EnsureUiTimer();
    wchar_t geomLine[256];
    swprintf_s(geomLine,
               L"Bound fill paint=%dx%d edge=%s appbar=%dx%d vertical=%d",
               target->pixelWidth, target->pixelHeight, EdgeName(target->geom.edge),
               target->geom.length, target->geom.thickness, target->geom.vertical ? 1 : 0);
    LogService(geomLine);
  }
}

void TaskbarAppearanceService::UnregisterHandle(uint64_t) {}

void TaskbarAppearanceService::Apply(const ThemeCommand& cmd) {
  {
    std::lock_guard lock(mutex_);
    command_ = cmd;
    start_ = std::chrono::steady_clock::now();
  }
  if (!dispatcher_) return;

  if (!cmd.enabled) {
    dispatcher_.TryEnqueue([this] { RestoreAll(); });
    return;
  }

  dispatcher_.TryEnqueue([this] {
    std::lock_guard lock(mutex_);
    for (auto& tv : taskbars_) {
      if (!tv.bound && tv.background && BindVisual(tv)) {
        boundCount_.fetch_add(1);
        SignalBound();
      } else if (tv.bound) {
        RefreshGeomAndBitmap(tv);
      }
    }
    EnsureUiTimer();
  });
}

void TaskbarAppearanceService::RestoreAll() {
  std::lock_guard lock(mutex_);
  if (timer_) {
    try {
      timer_.Stop();
    } catch (...) {
    }
  }
  for (auto& tv : taskbars_) {
    UnbindVisual(tv);
  }
  boundCount_.store(0);
}

void TaskbarAppearanceService::RefreshGeomAndBitmap(TaskbarVisual& tv) {
  if (!tv.background) return;
  int measuredW = 0, measuredH = 0;
  MeasureShape(tv.background, measuredW, measuredH);
  tv.geom = ResolveTaskbarGeomForSize(measuredW, measuredH);

  // Win11 often reports BackgroundFill as a tiny stub (e.g. 8×8) that the shell
  // then non-uniformly scales across the bar. Painting into that stub + Stretch::Fill
  // makes themes look stretched; Settings preview is fine because it uses real px.
  int w = measuredW;
  int h = measuredH;
  const int geomArea =
      (std::max)(1, tv.geom.length) * (std::max)(1, tv.geom.thickness);
  const int measuredArea = (std::max)(1, measuredW) * (std::max)(1, measuredH);
  const bool stubSize =
      (measuredW <= 32 && measuredH <= 32) ||
      (tv.geom.length >= 64 && tv.geom.thickness >= 8 && measuredArea * 16 < geomArea);
  if (stubSize && tv.geom.length >= 64 && tv.geom.thickness >= 8) {
    if (tv.geom.vertical) {
      w = tv.geom.thickness;
      h = tv.geom.length;
    } else {
      w = tv.geom.length;
      h = tv.geom.thickness;
    }
  } else {
    // Match composition pixels when XAML size is already the laid-out bar.
    double scale = 1.0;
    try {
      if (auto root = tv.background.XamlRoot()) {
        scale = root.RasterizationScale();
      }
    } catch (...) {
    }
    if (scale > 1.01) {
      w = (std::max)(8, static_cast<int>(std::lround(measuredW * scale)));
      h = (std::max)(8, static_cast<int>(std::lround(measuredH * scale)));
    }
  }
  w = (std::max)(8, (std::min)(w, 7680));
  h = (std::max)(8, (std::min)(h, 7680));

  if (w != tv.pixelWidth || h != tv.pixelHeight || !tv.bitmap) {
    try {
      tv.pixelWidth = w;
      tv.pixelHeight = h;
      tv.bitmap = wuxmi::WriteableBitmap(w, h);
      if (tv.imageBrush) {
        tv.imageBrush.ImageSource(tv.bitmap);
      }
    } catch (...) {
    }
  }
}

bool TaskbarAppearanceService::BindVisual(TaskbarVisual& tv) {
  if (!tv.background || tv.bound) return tv.bound;
  try {
    RefreshGeomAndBitmap(tv);
    if (!tv.bitmap) {
      tv.bitmap = wuxmi::WriteableBitmap(tv.pixelWidth, tv.pixelHeight);
    }

    wuxm::ImageBrush brush;
    brush.ImageSource(tv.bitmap);
    brush.Stretch(wuxm::Stretch::Fill);
    brush.AlignmentX(wuxm::AlignmentX::Center);
    brush.AlignmentY(wuxm::AlignmentY::Center);
    tv.imageBrush = brush;
    tv.background.Fill(brush);
    tv.bound = true;
    PaintVisual(tv, 0.f);
    return true;
  } catch (...) {
    UnbindVisual(tv);
    return false;
  }
}

void TaskbarAppearanceService::UnbindVisual(TaskbarVisual& tv) {
  if (tv.background && tv.originalFill) {
    try {
      tv.background.Fill(tv.originalFill);
    } catch (...) {
    }
  }
  tv.imageBrush = nullptr;
  tv.bitmap = nullptr;
  tv.pixelWidth = 0;
  tv.pixelHeight = 0;
  tv.bound = false;
}

void TaskbarAppearanceService::PaintVisual(TaskbarVisual& tv, float timeSeconds) {
  if (!tv.bound || !tv.bitmap || !command_.enabled) return;
  try {
    RefreshGeomAndBitmap(tv);
    auto buffer = tv.bitmap.PixelBuffer();
    uint8_t* data = BufferData(buffer);
    if (!data) return;

    // Orientation + paint metrics always follow the bitmap pixels so themes stay
    // undistorted regardless of display resolution / stub XAML sizes.
    const bool vertical = tv.pixelHeight > tv.pixelWidth * 3 / 2;
    PaintContext ctx;
    ctx.theme = static_cast<ThemeId>(command_.theme);
    ctx.intensity = command_.intensity / 100.f;
    ctx.timeSeconds = timeSeconds;
    ctx.vertical = vertical;
    ctx.length = vertical ? tv.pixelHeight : tv.pixelWidth;
    ctx.thickness = vertical ? tv.pixelWidth : tv.pixelHeight;
    if (vertical) {
      ctx.edge = (tv.geom.edge == TaskbarEdge::Right) ? TaskbarEdge::Right : TaskbarEdge::Left;
    } else {
      ctx.edge = (tv.geom.edge == TaskbarEdge::Top) ? TaskbarEdge::Top : TaskbarEdge::Bottom;
    }

    PaintThemePixels(ctx, data, tv.pixelWidth, tv.pixelHeight, tv.pixelWidth * 4);
    tv.bitmap.Invalidate();
  } catch (...) {
  }
}

void TaskbarAppearanceService::EnsureUiTimer() {
  if (!dispatcher_) return;
  if (!timer_) {
    timer_ = dispatcher_.CreateTimer();
    // ~24 fps is enough for calm waves and cheaper on the shell thread.
    timer_.Interval(std::chrono::milliseconds{42});
    timer_.IsRepeating(true);
    timer_.Tick([this](auto&&, auto&&) { TickUi(); });
  }
  if (!command_.enabled) {
    command_.enabled = 1;
    command_.theme = static_cast<uint32_t>(ThemeId::Rainbow);
    command_.intensity = 55;
    command_.apiVersion = kTapApiVersion;
  }
  try {
    timer_.Start();
  } catch (...) {
  }
  running_ = true;
}

void TaskbarAppearanceService::TickUi() {
  std::lock_guard lock(mutex_);
  if (!command_.enabled) return;
  const float t =
      std::chrono::duration<float>(std::chrono::steady_clock::now() - start_).count();
  for (auto& tv : taskbars_) {
    if (tv.bound) PaintVisual(tv, t);
  }
}

void TaskbarAppearanceService::StartRenderLoop() { EnsureUiTimer(); }

void TaskbarAppearanceService::StopRenderLoop() {
  running_ = false;
  if (dispatcher_) {
    dispatcher_.TryEnqueue([this] { RestoreAll(); });
    Sleep(80);
  } else {
    RestoreAll();
  }
}

void TaskbarAppearanceService::WatchHostMutex() {
  if (watchThread_.joinable()) return;
  watchThread_ = std::thread([this] {
    HANDLE mutex = nullptr;
    for (int i = 0; i < 40 && !mutex; ++i) {
      mutex = OpenMutexW(SYNCHRONIZE, FALSE, kMutexName.data());
      if (!mutex) Sleep(250);
    }
    if (!mutex) return;
    WaitForSingleObject(mutex, INFINITE);
    CloseHandle(mutex);
    StopRenderLoop();
  });
}

}  // namespace phostide
