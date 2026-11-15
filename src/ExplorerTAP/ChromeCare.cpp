#include "ChromeCare.h"
#include "TaskbarAppearanceService.h"

#include "../Common/MonitorEnum.h"

#include <algorithm>
#include <cmath>
#include <string>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#pragma pop_macro("GetCurrentTime")

namespace phostide {
namespace {

void LogChrome(const wchar_t* msg) {
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

std::wstring SafeName(winrt::Windows::UI::Xaml::FrameworkElement const& fe) {
  try {
    return std::wstring(fe.Name());
  } catch (...) {
    return {};
  }
}

std::wstring SafeTypeName(winrt::Windows::Foundation::IInspectable const& obj) {
  try {
    return std::wstring(winrt::get_class_name(obj));
  } catch (...) {
    return {};
  }
}

winrt::Windows::UI::Xaml::FrameworkElement ClockNudgeTarget(
    winrt::Windows::UI::Xaml::FrameworkElement const& fe) {
  // Prefer a nearby ancestor so nudge moves the whole clock button, not only glyphs.
  winrt::Windows::UI::Xaml::DependencyObject cur = fe;
  for (int i = 0; i < 6 && cur; ++i) {
    if (auto asFe = cur.try_as<winrt::Windows::UI::Xaml::FrameworkElement>()) {
      const auto type = SafeTypeName(asFe);
      if (type.find(L"Button") != std::wstring::npos ||
          type.find(L"Omni") != std::wstring::npos ||
          type.find(L"IconView") != std::wstring::npos) {
        return asFe;
      }
    }
    cur = winrt::Windows::UI::Xaml::Media::VisualTreeHelper::GetParent(cur);
  }
  return fe;
}

}  // namespace

bool EndsWithType(std::wstring_view type, std::wstring_view suffix) {
  // Prefer ends-with so TaskListButtonPanel does not match TaskListButton.
  return type.size() >= suffix.size() &&
         type.substr(type.size() - suffix.size()) == suffix &&
         (type.size() == suffix.size() || type[type.size() - suffix.size() - 1] == L'.');
}

bool IsSearchChromeType(std::wstring_view type) {
  // Win11 Search pill variants (icon / box / pill) — not ExperienceToggleButton.
  return EndsWithType(type, L"Taskbar.SearchBoxButton") ||
         EndsWithType(type, L"SearchUx.SearchUI.SearchBoxButton") ||
         EndsWithType(type, L"SearchUx.SearchUI.SearchPillButton") ||
         EndsWithType(type, L"SearchUx.SearchUI.SearchIconButton");
}

bool ChromeNameIsWhitelisted(std::wstring_view name) {
  return name == L"LaunchListButton" || name == L"SystemTrayFrameGrid" ||
         name == L"TimeInnerTextBlock" || name == L"DateInnerTextBlock" ||
         name == L"SearchBoxButton" || name == L"SearchBox";
}

bool ChromeTypeIsWhitelisted(std::wstring_view type) {
  return EndsWithType(type, L"Taskbar.TaskListButton") ||
         EndsWithType(type, L"Taskbar.ExperienceToggleButton") ||
         EndsWithType(type, L"SystemTray.SystemTrayFrame") || IsSearchChromeType(type);
}

ChromeKind ClassifyChrome(std::wstring_view name, std::wstring_view type) {
  if (name == L"TimeInnerTextBlock" || name == L"DateInnerTextBlock") {
    return ChromeKind::Clock;
  }
  if (name == L"SystemTrayFrameGrid" || EndsWithType(type, L"SystemTray.SystemTrayFrame")) {
    return ChromeKind::Tray;
  }
  return ChromeKind::Icons;
}

bool ResolveChromeMonitor(ChromeVisual& cv) {
  if (cv.trayHwnd && !cv.deviceHash) {
    phostide::DeviceHashFromWindow(cv.trayHwnd, cv.deviceHash);
  }
  if (!cv.deviceHash && cv.element) {
    // Advise path registers chrome with trayHwnd=null; bind via shared XamlRoot
    // with the BackgroundFill on that same taskbar island.
    cv.deviceHash = TaskbarAppearanceService::Instance().DeviceHashForElement(cv.element);
    if (cv.deviceHash && !cv.trayHwnd) {
      cv.trayHwnd =
          TaskbarAppearanceService::Instance().TrayHwndForDeviceHash(cv.deviceHash);
    }
  }
  return cv.deviceHash != 0;
}

bool SlotCareActive(const ThemeCommand& cmd, uint64_t deviceHash) {
  // v6: strictly per-slot. Never fall back to global careActive OR — that dimmed
  // chrome on every bar whenever any one display was idle.
  if (cmd.apiVersion >= 6 && cmd.slotCount > 0) {
    if (deviceHash == 0) return false;
    const uint32_t n = (std::min)(cmd.slotCount, kMaxMonitorSlots);
    for (uint32_t i = 0; i < n; ++i) {
      if (cmd.slots[i].deviceHash == deviceHash) {
        return cmd.slots[i].careActive != 0 && cmd.slots[i].enabled != 0;
      }
    }
    return false;
  }
  return cmd.careActive != 0;
}

ChromeCareService& ChromeCareService::Instance() {
  static ChromeCareService inst;
  return inst;
}

bool ChromeCareService::TryRegister(winrt::Windows::UI::Xaml::FrameworkElement const& fe,
                                    HWND trayHwnd, uint64_t diagnosticsHandle) {
  if (!fe) return false;
  const auto name = SafeName(fe);
  const auto type = SafeTypeName(fe);
  const bool byName = ChromeNameIsWhitelisted(name);
  const bool byType = ChromeTypeIsWhitelisted(type);
  if (!byName && !byType) return false;

  ChromeKind kind = ClassifyChrome(name, type);
  winrt::Windows::UI::Xaml::FrameworkElement target = fe;
  if (kind == ChromeKind::Clock &&
      (name == L"TimeInnerTextBlock" || name == L"DateInnerTextBlock")) {
    // Register nudge/dim on the clock host; still dim the text blocks themselves too.
  }

  std::lock_guard lock(mutex_);
  for (auto& existing : elements_) {
    const bool same = existing.element == fe ||
                      (diagnosticsHandle && existing.diagnosticsHandle == diagnosticsHandle);
    if (!same) continue;
    // Hit-test often arrives after Advise(null tray) — upgrade mapping in place.
    if (trayHwnd && (!existing.trayHwnd || !existing.deviceHash)) {
      existing.trayHwnd = trayHwnd;
      phostide::DeviceHashFromWindow(trayHwnd, existing.deviceHash);
    }
    return true;
  }

  ChromeVisual cv;
  cv.element = (kind == ChromeKind::Clock && name == L"TimeInnerTextBlock")
                   ? ClockNudgeTarget(fe)
                   : fe;
  // For DateInnerTextBlock keep the text element so both time+date dim; nudge uses Time path.
  if (kind == ChromeKind::Clock && name == L"DateInnerTextBlock") {
    cv.element = fe;
  }
  try {
    cv.originalOpacity = cv.element.Opacity();
  } catch (...) {
    cv.originalOpacity = 1.0;
  }
  try {
    cv.originalTransform = cv.element.RenderTransform();
  } catch (...) {
    cv.originalTransform = nullptr;
  }
  cv.kind = kind;
  cv.diagnosticsHandle = diagnosticsHandle;
  cv.trayHwnd = trayHwnd;
  if (trayHwnd) {
    phostide::DeviceHashFromWindow(trayHwnd, cv.deviceHash);
  } else {
    ResolveChromeMonitor(cv);
  }
  elements_.push_back(std::move(cv));

  wchar_t line[256];
  swprintf_s(line, L"Chrome registered kind=%d name=%s type=%s hash=%llu tray=%p",
             static_cast<int>(kind), name.c_str(), type.c_str(),
             static_cast<unsigned long long>(elements_.back().deviceHash),
             static_cast<void*>(elements_.back().trayHwnd));
  LogChrome(line);
  return true;
}

void ChromeCareService::UnregisterHandle(uint64_t diagnosticsHandle) {
  if (!diagnosticsHandle) return;
  std::lock_guard lock(mutex_);
  for (auto it = elements_.begin(); it != elements_.end(); ++it) {
    if (it->diagnosticsHandle != diagnosticsHandle) continue;
    RestoreOne(*it);
    elements_.erase(it);
    return;
  }
}

void ChromeCareService::RestoreOne(ChromeVisual& cv) {
  if (!cv.element) return;
  try {
    if (cv.dimApplied) {
      cv.element.Opacity(cv.originalOpacity);
      cv.dimApplied = false;
    }
  } catch (...) {
  }
  try {
    if (cv.nudgeApplied) {
      cv.element.RenderTransform(cv.originalTransform);
      cv.nudgeApplied = false;
    }
  } catch (...) {
  }
}

void ChromeCareService::ApplyOne(ChromeVisual& cv, const ThemeCommand& cmd,
                                 float timeSeconds) {
  if (!cv.element) return;
  ResolveChromeMonitor(cv);
  const bool care = SlotCareActive(cmd, cv.deviceHash);
  const bool chromeDim = care && (cmd.careFlags & kCareFlagChromeDim) != 0;
  const bool clockNudge = care && (cmd.careFlags & kCareFlagClockNudge) != 0 &&
                          cv.kind == ChromeKind::Clock;

  if (!chromeDim) {
    if (cv.dimApplied) {
      try {
        cv.element.Opacity(cv.originalOpacity);
      } catch (...) {
      }
      cv.dimApplied = false;
    }
  } else {
    const double op = (cv.kind == ChromeKind::Clock)
                          ? (cmd.clockOpacity / 100.0)
                          : (cmd.chromeOpacity / 100.0);
    try {
      cv.element.Opacity(op);
      cv.dimApplied = true;
      if (!cv.deviceHash) {
        LogChrome(L"Chrome dim applied (no deviceHash)");
      }
    } catch (...) {
      LogChrome(L"Chrome dim Opacity failed");
    }
  }

  if (!clockNudge) {
    if (cv.nudgeApplied) {
      try {
        cv.element.RenderTransform(cv.originalTransform);
      } catch (...) {
      }
      cv.nudgeApplied = false;
    }
    return;
  }

  // Slow Lissajous-ish micro nudge (±~1.5 px) — only while idle care is on.
  const float x = 1.5f * std::sin(timeSeconds * 0.23f);
  const float y = 1.2f * std::cos(timeSeconds * 0.17f);
  try {
    winrt::Windows::UI::Xaml::Media::TranslateTransform tt;
    tt.X(x);
    tt.Y(y);
    cv.element.RenderTransform(tt);
    cv.nudgeApplied = true;
  } catch (...) {
  }
}

void ChromeCareService::Apply(const ThemeCommand& cmd) {
  std::lock_guard lock(mutex_);
  command_ = cmd;
  const float t = 0.f;
  for (auto& cv : elements_) {
    ApplyOne(cv, command_, t);
  }
}

void ChromeCareService::TickNudge(float timeSeconds) {
  std::lock_guard lock(mutex_);
  // careActive is OR of per-slot care (v6); ApplyOne filters by tray monitor.
  if (command_.careActive == 0 || (command_.careFlags & kCareFlagClockNudge) == 0) {
    return;
  }
  for (auto& cv : elements_) {
    if (cv.kind != ChromeKind::Clock) continue;
    ApplyOne(cv, command_, timeSeconds);
  }
}

void ChromeCareService::RestoreAll() {
  std::lock_guard lock(mutex_);
  for (auto& cv : elements_) {
    RestoreOne(cv);
  }
  elements_.clear();
  command_ = {};
  LogChrome(L"Chrome RestoreAll done");
}

}  // namespace phostide
