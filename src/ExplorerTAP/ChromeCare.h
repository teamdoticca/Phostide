#pragma once

#include "../Common/PhostideCommon.h"

#include <mutex>
#include <vector>

#include <windows.h>

#pragma push_macro("GetCurrentTime")
#undef GetCurrentTime
#include <winrt/Windows.UI.Xaml.h>
#include <winrt/Windows.UI.Xaml.Media.h>
#pragma pop_macro("GetCurrentTime")

namespace phostide {

enum class ChromeKind {
  Icons,
  Tray,
  Clock,
};

struct ChromeVisual {
  winrt::Windows::UI::Xaml::FrameworkElement element{nullptr};
  double originalOpacity = 1.0;
  winrt::Windows::UI::Xaml::Media::Transform originalTransform{nullptr};
  ChromeKind kind = ChromeKind::Icons;
  uint64_t diagnosticsHandle = 0;
  HWND trayHwnd = nullptr;
  uint64_t deviceHash = 0;  // monitor for this tray — per-slot careActive (v6)
  bool dimApplied = false;
  bool nudgeApplied = false;
};

// Idle chrome dim + clock micro-nudge for whitelisted Explorer taskbar elements.
class ChromeCareService {
 public:
  static ChromeCareService& Instance();

  // Returns true if the FrameworkElement matched the OLED Care whitelist.
  bool TryRegister(winrt::Windows::UI::Xaml::FrameworkElement const& fe, HWND trayHwnd,
                   uint64_t diagnosticsHandle = 0);
  void UnregisterHandle(uint64_t diagnosticsHandle);
  void Apply(const ThemeCommand& cmd);
  void TickNudge(float timeSeconds);
  void RestoreAll();

 private:
  ChromeCareService() = default;

  void RestoreOne(ChromeVisual& cv);
  void ApplyOne(ChromeVisual& cv, const ThemeCommand& cmd, float timeSeconds);

  std::mutex mutex_;
  std::vector<ChromeVisual> elements_;
  ThemeCommand command_{};
};

bool ChromeNameIsWhitelisted(std::wstring_view name);
bool ChromeTypeIsWhitelisted(std::wstring_view type);
ChromeKind ClassifyChrome(std::wstring_view name, std::wstring_view type);

}  // namespace phostide
