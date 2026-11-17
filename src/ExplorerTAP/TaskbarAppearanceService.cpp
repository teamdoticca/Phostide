#include "TaskbarAppearanceService.h"

#include "ChromeCare.h"

#include "../Common/MonitorEnum.h"
#include "GpuThemeRenderer.h"
#include "PixelThemes.h"
#include "TapSite.h"
#include "TaskbarGeometry.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <thread>

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
  // Stamp the running TAP build so the host can detect a stale in-Explorer DLL
  // after an upgrade (XAML Diagnostics pins the module until Explorer restarts).
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  wcscat_s(temp, L"Phostide");
  CreateDirectoryW(temp, nullptr);
  wchar_t path[MAX_PATH]{};
  swprintf_s(path, L"%s\\%s", temp, kTapVersionStamp.data());
  HANDLE f = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f != INVALID_HANDLE_VALUE) {
    char line[64]{};
    const int n = WideCharToMultiByte(CP_UTF8, 0, kAppVersion.data(),
                                      static_cast<int>(kAppVersion.size()), line,
                                      static_cast<int>(sizeof(line) - 2), nullptr, nullptr);
    if (n > 0) {
      line[n] = '\n';
      DWORD written = 0;
      WriteFile(f, line, static_cast<DWORD>(n + 1), &written, nullptr);
    }
    CloseHandle(f);
  }
}

void ClearBoundEvent() {
  if (HANDLE ev = OpenEventW(EVENT_MODIFY_STATE, FALSE, kTapBoundEvent.data())) {
    ResetEvent(ev);
    CloseHandle(ev);
  } else if (HANDLE ev = CreateEventW(nullptr, TRUE, FALSE, kTapBoundEvent.data())) {
    ResetEvent(ev);
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

void TaskbarAppearanceService::RegisterBackground(wuxs::Shape const& shape, HWND trayHwnd,
                                                  uint64_t diagnosticsHandle) {
  if (!shape) return;
  if (!dispatcher_) {
    dispatcher_ = wsys::DispatcherQueue::GetForCurrentThread();
  }

  TaskbarVisual* target = nullptr;
  bool trayCorrected = false;
  bool needsBind = false;
  {
    std::lock_guard lock(mutex_);
    for (auto& tv : taskbars_) {
      if (tv.background == shape) {
        // Hit-test registration knows the real tray HWND; prefer it over size guess.
        if (trayHwnd && tv.trayHwnd != trayHwnd) {
          tv.trayHwnd = trayHwnd;
          trayCorrected = true;
        }
        if (diagnosticsHandle && tv.diagnosticsHandle != diagnosticsHandle) {
          tv.diagnosticsHandle = diagnosticsHandle;
        }
        target = &tv;
        // After SoftSuspend, the shape is still listed but unbound — must Bind again.
        needsBind = !tv.bound || (!tv.gpuActive && !tv.imageBrush);
        break;
      }
    }
    if (!target) {
      TaskbarVisual tv;
      tv.background = shape;
      tv.trayHwnd = trayHwnd;
      tv.diagnosticsHandle = diagnosticsHandle;
      try {
        tv.originalFill = shape.Fill();
      } catch (...) {
      }
      taskbars_.push_back(std::move(tv));
      target = &taskbars_.back();
      needsBind = true;
    }
  }

  if (trayCorrected && target && !needsBind) {
    std::lock_guard lock(mutex_);
    RefreshGeomAndBitmap(*target);
    wchar_t line[192];
    swprintf_s(line, L"tray hwnd corrected hash=%llu hwnd=%p",
               static_cast<unsigned long long>(target->deviceHash),
               static_cast<void*>(target->trayHwnd));
    LogService(line);
    return;
  }

  if (target && needsBind && BindVisual(*target)) {
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
    wchar_t geomLine[320];
    swprintf_s(geomLine,
               L"Bound fill paint=%dx%d edge=%s appbar=%dx%d vertical=%d hash=%llu hwnd=%p",
               target->pixelWidth, target->pixelHeight, EdgeName(target->geom.edge),
               target->geom.length, target->geom.thickness, target->geom.vertical ? 1 : 0,
               static_cast<unsigned long long>(target->deviceHash),
               static_cast<void*>(target->trayHwnd));
    LogService(geomLine);
  }
}

void TaskbarAppearanceService::UnregisterHandle(uint64_t diagnosticsHandle) {
  if (!diagnosticsHandle) return;
  // Soft-suspend Unbind (Fill restore / GPU Detach) can emit spurious Remove mutations
  // for the same BackgroundFill handle. Dropping the ref here is what broke tray rebind.
  if (suspended_.load()) {
    LogService(L"UnregisterHandle ignored (suspended)");
    return;
  }
  std::lock_guard lock(mutex_);
  for (auto it = taskbars_.begin(); it != taskbars_.end(); ++it) {
    if (it->diagnosticsHandle != diagnosticsHandle) continue;
    const bool wasBound = it->bound;
    UnbindVisual(*it);
    if (wasBound) boundCount_.fetch_sub(1);
    taskbars_.erase(it);
    LogService(L"UnregisterHandle removed BackgroundFill");
    return;
  }
}

void TaskbarAppearanceService::RefreshDeviceHash(TaskbarVisual& tv) {
  std::wstring name;
  if (MonitorDeviceNameFromRect(tv.geom.rc, name)) {
    tv.deviceHash = HashMonitorDeviceName(name);
  } else {
    tv.deviceHash = 0;
  }
}

uint64_t TaskbarAppearanceService::DeviceHashForElement(
    winrt::Windows::UI::Xaml::FrameworkElement const& fe) const {
  if (!fe) return 0;
  try {
    const auto root = fe.XamlRoot();
    if (!root) return 0;
    std::lock_guard lock(mutex_);
    for (const auto& tv : taskbars_) {
      if (!tv.background || !tv.deviceHash) continue;
      try {
        if (tv.background.XamlRoot() == root) return tv.deviceHash;
      } catch (...) {
      }
    }
  } catch (...) {
  }
  return 0;
}

HWND TaskbarAppearanceService::TrayHwndForDeviceHash(uint64_t deviceHash) const {
  if (!deviceHash) return nullptr;
  std::lock_guard lock(mutex_);
  for (const auto& tv : taskbars_) {
    if (tv.deviceHash == deviceHash && tv.trayHwnd) return tv.trayHwnd;
  }
  return nullptr;
}

const MonitorPaintSlot* TaskbarAppearanceService::SlotFor(const TaskbarVisual& tv) const {
  if (command_.apiVersion < 4 || command_.slotCount == 0 || tv.deviceHash == 0) {
    return nullptr;
  }
  const uint32_t n = (std::min)(command_.slotCount, kMaxMonitorSlots);
  for (uint32_t i = 0; i < n; ++i) {
    if (command_.slots[i].deviceHash == tv.deviceHash) return &command_.slots[i];
  }
  return nullptr;
}

bool TaskbarAppearanceService::SlotEnabled(const TaskbarVisual& tv) const {
  if (const auto* slot = SlotFor(tv)) return slot->enabled != 0;
  return true;
}

bool TaskbarAppearanceService::SlotPauseActive(const TaskbarVisual& tv) const {
  if (command_.apiVersion < 7) return false;
  if (const auto* slot = SlotFor(tv)) return slot->pauseActive != 0;
  return false;
}

void TaskbarAppearanceService::AppearanceFor(const TaskbarVisual& tv, ThemeId& theme,
                                             uint32_t& intensity) const {
  if (const auto* slot = SlotFor(tv)) {
    theme = static_cast<ThemeId>(slot->theme);
    intensity = slot->intensity;
    return;
  }
  theme = static_cast<ThemeId>(command_.theme);
  intensity = command_.intensity;
}

void TaskbarAppearanceService::Apply(const ThemeCommand& cmd) {
  {
    std::lock_guard lock(mutex_);
    const ThemeCommand prev = command_;
    const auto prevMode = static_cast<RenderMode>(prev.renderMode);
    const bool wasLive = prev.enabled != 0 && prev.paused == 0;
    command_ = cmd;
    const auto mode = static_cast<RenderMode>(command_.renderMode);
    if (mode != prevMode && mode != RenderMode::Cpu) {
      gpuFailedThisSession_ = false;
    }
    // Care / intensity polls must NOT rewind animation time — that looks like a
    // full theme reapply and hides the idle dim. Only restart the clock when
    // paint actually (re)starts or the theme identity changes.
    const bool nowLive = cmd.enabled != 0 && cmd.paused == 0;
    bool themeChanged = prev.theme != cmd.theme;
    if (!themeChanged) {
      const uint32_t n = (std::min)((std::min)(prev.slotCount, cmd.slotCount), kMaxMonitorSlots);
      for (uint32_t i = 0; i < n; ++i) {
        if (prev.slots[i].deviceHash == cmd.slots[i].deviceHash &&
            prev.slots[i].theme != cmd.slots[i].theme) {
          themeChanged = true;
          break;
        }
      }
    }
    if (nowLive && (!wasLive || themeChanged)) {
      start_ = std::chrono::steady_clock::now();
    }
  }
  {
    wchar_t line[192];
    swprintf_s(line, L"Apply enabled=%u paused=%u slots=%u theme=%u intensity=%u",
               cmd.enabled ? 1u : 0u, cmd.paused ? 1u : 0u, cmd.slotCount, cmd.theme,
               cmd.intensity);
    LogService(line);
    const uint32_t n = (std::min)(cmd.slotCount, kMaxMonitorSlots);
    for (uint32_t i = 0; i < n; ++i) {
      wchar_t slotLine[160];
      swprintf_s(slotLine,
                 L"  slot[%u] hash=%llu theme=%u intensity=%u enabled=%u care=%u pause=%u",
                 i, static_cast<unsigned long long>(cmd.slots[i].deviceHash),
                 cmd.slots[i].theme, cmd.slots[i].intensity, cmd.slots[i].enabled,
                 cmd.slots[i].careActive, cmd.slots[i].pauseActive);
      LogService(slotLine);
    }
  }
  if (!dispatcher_) return;

  if (!cmd.enabled) {
    // Soft pause (tray Enable toggle): keep shape refs so re-enable can BindVisual
    // without a fragile BackgroundFill rescan. Full RestoreAll is for host Exit only.
    dispatcher_.TryEnqueue([this] { SuspendPainting(); });
    return;
  }

  suspended_.store(false);

  if (cmd.paused) {
    dispatcher_.TryEnqueue([this, cmd] {
      // Never hold mutex_ while calling ChromeCare (opposite order from TickUi).
      {
        std::lock_guard lock(mutex_);
        StopUiTimer();
      }
      ChromeCareService::Instance().Apply(cmd);
    });
    return;
  }

  dispatcher_.TryEnqueue([this, cmd] {
    bool needRescan = false;
    int kept = 0;
    int rebound = 0;
    int live = 0;
    {
      std::lock_guard lock(mutex_);
      kept = static_cast<int>(taskbars_.size());
      needRescan = taskbars_.empty();
      for (auto& tv : taskbars_) {
        if (!tv.background) continue;
        RefreshGeomAndBitmap(tv);
        const bool wasBound = tv.bound;
        if (!SlotEnabled(tv)) {
          if (wasBound) {
            UnbindVisual(tv);
            boundCount_.fetch_sub(1);
          }
          continue;
        }
        // Live paint only if we still own a GPU target or ImageBrush. SoftSuspend
        // can leave bound=true with no brush after a failed partial unbind — then
        // skipping BindVisual leaves a stock Windows fill forever.
        const bool livePaint = wasBound && (tv.gpuActive || tv.imageBrush);
        if (livePaint) {
          PaintVisual(tv, std::chrono::duration<float>(
                              std::chrono::steady_clock::now() - start_)
                              .count());
          ++live;
          continue;
        }
        if (wasBound) {
          UnbindVisual(tv);
          boundCount_.fetch_sub(1);
        }
        if (BindVisual(tv)) {
          boundCount_.fetch_add(1);
          SignalBound();
          ++rebound;
        }
      }
      EnsureUiTimer();
    }
    {
      wchar_t line[160];
      swprintf_s(line, L"Apply rebind kept=%d live=%d rebound=%d needRescan=%d", kept, live,
                 rebound, needRescan ? 1 : 0);
      LogService(line);
    }
    // Chrome Care after releasing paint mutex — avoids deadlock with TickUi.
    ChromeCareService::Instance().Apply(cmd);
    // Same-process tray rebuild: fills were cleared / never registered — re-hit-test.
    if (needRescan) {
      LogService(L"Apply: no taskbars — RequestRescan");
      TAPSite::RequestRescan();
      // After host Exit→RestoreAll, the first hit-test often misses BackgroundFill.
      // Retry on the dispatcher a few times so reinject without Explorer bounce recovers.
      std::thread([this] {
        for (int i = 0; i < 5; ++i) {
          Sleep(350 + static_cast<DWORD>(i) * 250);
          if (HasBoundFill()) return;
          if (suspended_.load()) return;
          if (!command_.enabled) return;
          if (!dispatcher_) return;
          dispatcher_.TryEnqueue([] {
            LogService(L"Apply: delayed RequestRescan");
            TAPSite::RequestRescan();
          });
        }
      }).detach();
    }
  });
}

void TaskbarAppearanceService::StopUiTimer() {
  running_ = false;
  if (timer_) {
    try {
      timer_.Stop();
    } catch (...) {
    }
  }
}

void TaskbarAppearanceService::SuspendPainting() {
  // Set before Unbind so VisualTree Remove callbacks do not erase fill refs.
  suspended_.store(true);
  int kept = 0;
  {
    std::lock_guard lock(mutex_);
    StopUiTimer();
    kept = static_cast<int>(taskbars_.size());
    for (auto& tv : taskbars_) {
      if (!tv.bound) continue;
      UnbindVisual(tv);
      boundCount_.fetch_sub(1);
    }
  }
  ClearBoundEvent();
  ThemeCommand careOff = command_;
  careOff.careActive = 0;
  careOff.careFlags = 0;
  ChromeCareService::Instance().Apply(careOff);
  wchar_t line[96];
  swprintf_s(line, L"SuspendPainting kept=%d fill refs for rebind", kept);
  LogService(line);
}

void TaskbarAppearanceService::RestoreAll() {
  suspended_.store(false);
  {
    std::lock_guard lock(mutex_);
    StopUiTimer();
    for (auto& tv : taskbars_) {
      UnbindVisual(tv);
    }
    gpu_.DetachAll();
    taskbars_.clear();
    boundCount_.store(0);
  }
  ClearBoundEvent();
  // Outside mutex_: TickUi must never wait on ChromeCare while holding mutex_.
  ChromeCareService::Instance().RestoreAll();
  LogService(L"RestoreAll done");
}

bool TaskbarAppearanceService::WantGpu(const TaskbarVisual& /*tv*/) const {
  if (gpuFailedThisSession_) return false;
  const auto mode = static_cast<RenderMode>(command_.renderMode);
  return mode == RenderMode::Auto || mode == RenderMode::Gpu;
}

bool TaskbarAppearanceService::TryEnableGpuFor(TaskbarVisual& tv) {
  if (!WantGpu(tv) || !tv.background || tv.pixelWidth < 1 || tv.pixelHeight < 1) {
    return false;
  }
  if (!gpu_.EnsureDevice()) {
    gpuFailedThisSession_ = true;
    return false;
  }
  if (!gpu_.Attach(tv.background, tv.pixelWidth, tv.pixelHeight)) {
    gpuFailedThisSession_ = true;
    return false;
  }
  tv.gpuActive = true;
  tv.imageBrush = nullptr;
  tv.bitmap = nullptr;
  return true;
}

void TaskbarAppearanceService::RefreshGeomAndBitmap(TaskbarVisual& tv) {
  if (!tv.background) return;
  int measuredW = 0, measuredH = 0;
  MeasureShape(tv.background, measuredW, measuredH);

  if (tv.trayHwnd && IsWindow(tv.trayHwnd)) {
    tv.geom = GeomFromTrayHwnd(tv.trayHwnd);
  } else {
    HWND skipped[kMaxMonitorSlots]{};
    size_t skippedCount = 0;
    for (const auto& other : taskbars_) {
      if (&other == &tv || !other.trayHwnd) continue;
      if (skippedCount < kMaxMonitorSlots) skipped[skippedCount++] = other.trayHwnd;
    }
    HWND matched = nullptr;
    tv.geom =
        ResolveTaskbarGeomForSize(measuredW, measuredH, skipped, skippedCount, &matched);
    if (matched) tv.trayHwnd = matched;
  }
  RefreshDeviceHash(tv);

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

  // Cap the long axis so CPU theme painters stay cheap; ImageBrush Stretch::Fill
  // upscales to the real BackgroundFill. Thickness keeps full detail.
  // GPU paints at native measured/DPI size (hard clamp 7680 above) — no 2560 cap.
  if (!WantGpu(tv)) {
    constexpr int kMaxCpuPaintAlongPx = 720;
    if (h > w * 3 / 2) {
      if (h > kMaxCpuPaintAlongPx) h = kMaxCpuPaintAlongPx;
    } else if (w > kMaxCpuPaintAlongPx) {
      w = kMaxCpuPaintAlongPx;
    }
  }

  const bool sizeChanged = (w != tv.pixelWidth || h != tv.pixelHeight);
  tv.pixelWidth = w;
  tv.pixelHeight = h;

  if (tv.gpuActive) {
    if (sizeChanged && tv.background) {
      gpu_.Resize(tv.background, w, h);
    }
    return;
  }

  if (sizeChanged || !tv.bitmap) {
    try {
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
  if (suspended_.load() || !command_.enabled) return false;
  try {
    RefreshGeomAndBitmap(tv);
    if (!SlotEnabled(tv)) return false;
    tv.bound = true;

    if (TryEnableGpuFor(tv)) {
      PaintVisual(tv, 0.f);
      EnsureUiTimer();  // switch to GPU ~55 Hz tick
      return true;
    }

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
    tv.gpuActive = false;
    PaintVisual(tv, 0.f);
    return true;
  } catch (...) {
    UnbindVisual(tv);
    return false;
  }
}

void TaskbarAppearanceService::UnbindVisual(TaskbarVisual& tv) {
  if (tv.gpuActive && tv.background) {
    gpu_.Detach(tv.background);
    tv.gpuActive = false;
  }
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

void TaskbarAppearanceService::PaintVisualCpu(TaskbarVisual& tv, float timeSeconds) {
  if (!tv.bitmap) return;
  try {
    auto buffer = tv.bitmap.PixelBuffer();
    uint8_t* data = BufferData(buffer);
    if (!data) return;

    ThemeId theme{};
    uint32_t intensity = 45;
    AppearanceFor(tv, theme, intensity);

    const bool vertical = tv.pixelHeight > tv.pixelWidth * 3 / 2;
    PaintContext ctx;
    ctx.theme = theme;
    ctx.intensity = intensity / 100.f;
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

void TaskbarAppearanceService::PaintVisual(TaskbarVisual& tv, float timeSeconds) {
  if (!tv.bound || !command_.enabled || command_.paused || !SlotEnabled(tv) ||
      SlotPauseActive(tv)) {
    return;
  }

  ThemeId theme{};
  uint32_t intensity = 45;
  AppearanceFor(tv, theme, intensity);

  const bool vertical = tv.pixelHeight > tv.pixelWidth * 3 / 2;
  TaskbarEdge edge = TaskbarEdge::Bottom;
  if (vertical) {
    edge = (tv.geom.edge == TaskbarEdge::Right) ? TaskbarEdge::Right : TaskbarEdge::Left;
  } else {
    edge = (tv.geom.edge == TaskbarEdge::Top) ? TaskbarEdge::Top : TaskbarEdge::Bottom;
  }

  if (tv.gpuActive) {
    const bool ok =
        gpu_.Paint(tv.background, theme, intensity / 100.f, timeSeconds, edge, tv.pixelWidth,
                   tv.pixelHeight);
    if (ok) return;
    // Fall back to CPU for this visual for the rest of the session.
    tv.gpuActive = false;
    gpuFailedThisSession_ = true;
    gpu_.Detach(tv.background);
    try {
      if (!tv.bitmap) {
        tv.bitmap = wuxmi::WriteableBitmap(tv.pixelWidth, tv.pixelHeight);
      }
      wuxm::ImageBrush brush;
      brush.ImageSource(tv.bitmap);
      brush.Stretch(wuxm::Stretch::Fill);
      tv.imageBrush = brush;
      tv.background.Fill(brush);
    } catch (...) {
      EnsureUiTimer();
      return;
    }
    EnsureUiTimer();  // drop back to CPU interval
  }

  PaintVisualCpu(tv, timeSeconds);
}

void TaskbarAppearanceService::EnsureUiTimer() {
  if (!dispatcher_) return;
  if (!timer_) {
    timer_ = dispatcher_.CreateTimer();
    timer_.IsRepeating(true);
    timer_.Tick([this](auto&&, auto&&) { TickUi(); });
  }
  if (!command_.enabled || command_.paused) {
    StopUiTimer();
    return;
  }
  // Stop the loop when every live slot is fullscreen-paused (free GPU for the game).
  bool anyNeedsPaint = false;
  for (const auto& tv : taskbars_) {
    if (tv.bound && SlotEnabled(tv) && !SlotPauseActive(tv)) {
      anyNeedsPaint = true;
      break;
    }
  }
  if (!anyNeedsPaint) {
    StopUiTimer();
    return;
  }
  // GPU path is cheap — ~55 Hz. CPU keeps slower theme-tuned intervals.
  bool anyGpu = false;
  for (const auto& tv : taskbars_) {
    if (tv.bound && tv.gpuActive && !SlotPauseActive(tv)) {
      anyGpu = true;
      break;
    }
  }
  int intervalMs = 18;  // ~55 Hz
  if (!anyGpu) {
    intervalMs = 90;
    for (const auto& tv : taskbars_) {
      if (!tv.bound) continue;
      ThemeId theme{};
      uint32_t intensity = 45;
      AppearanceFor(tv, theme, intensity);
      if (theme == ThemeId::Aurora || theme == ThemeId::SeaWaves ||
          theme == ThemeId::Weather) {
        intervalMs = 110;
        break;
      }
    }
  }
  try {
    timer_.Interval(std::chrono::milliseconds{intervalMs});
    timer_.Start();
  } catch (...) {
  }
  running_ = true;
}

void TaskbarAppearanceService::TickUi() {
  float t = 0.f;
  bool nudge = false;
  {
    std::lock_guard lock(mutex_);
    if (!command_.enabled || command_.paused) return;
    // Occasional remeasure catches DPI / monitor changes SizeChanged may miss.
    if ((++paintFrame_ % 90u) == 0u) {
      for (auto& tv : taskbars_) {
        if (tv.bound) RefreshGeomAndBitmap(tv);
      }
    }
    t = std::chrono::duration<float>(std::chrono::steady_clock::now() - start_).count();
    for (auto& tv : taskbars_) {
      if (!tv.background) continue;
      if (!SlotEnabled(tv)) {
        if (tv.bound) {
          UnbindVisual(tv);
          boundCount_.fetch_sub(1);
        }
        continue;
      }
      if (tv.bound && !SlotPauseActive(tv)) PaintVisual(tv, t);
    }
    nudge = command_.careActive != 0 && (command_.careFlags & kCareFlagClockNudge) != 0;
  }
  // Outside mutex_ — Apply/RestoreAll also touch ChromeCare; never nest both locks.
  if (nudge) ChromeCareService::Instance().TickNudge(t);
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
    // Re-arm across host restarts while Explorer keeps this TAP loaded.
    // After the host exits, wait until the named mutex disappears before
    // watching again — an unowned leftover mutex would make Wait return
    // immediately and spin RestoreAll.
    for (;;) {
      HANDLE mutex = nullptr;
      while (!mutex) {
        mutex = OpenMutexW(SYNCHRONIZE, FALSE, kMutexName.data());
        if (!mutex) Sleep(250);
      }
      // Wait returns OBJECT_0 / ABANDONED with ownership. Always Release before
      // Close — otherwise TAP keeps the SingleInstance mutex forever and the next
      // Phostide.exe sees ERROR_ALREADY_EXISTS and exits without painting.
      const DWORD wait = WaitForSingleObject(mutex, INFINITE);
      if (wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED) {
        ReleaseMutex(mutex);
      }
      CloseHandle(mutex);
      // Soft-suspend: restore stock fills but KEEP BackgroundFill refs so the next
      // Phostide.exe can BindVisual without a fragile hit-test rescan. Full
      // RestoreAll (clear) made host-only restarts fall back to accent-only tint.
      LogService(L"Host mutex released - soft suspend for rebind");
      running_ = false;
      if (dispatcher_) {
        dispatcher_.TryEnqueue([this] { SuspendPainting(); });
        Sleep(80);
      } else {
        SuspendPainting();
      }
      // Wait until no handles remain so CreateMutex in the next host is a fresh
      // create (err=0), not ERROR_ALREADY_EXISTS from our probe/watch handle.
      for (;;) {
        HANDLE probe = OpenMutexW(SYNCHRONIZE, FALSE, kMutexName.data());
        if (!probe) break;
        CloseHandle(probe);
        Sleep(250);
      }
    }
  });
}

}  // namespace phostide
