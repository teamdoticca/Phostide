#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace phostide {

struct UpdateCheckResult {
  bool ok = false;                 // network + parse succeeded
  bool updateAvailable = false;    // latest tag newer than kAppVersion
  std::wstring latestTag;          // e.g. "v0.1.3"
  bool prerelease = false;         // latestTag is a GitHub pre-release
  std::wstring setupUrl;           // Setup asset URL or release page
  std::wstring releasePageUrl;     // html_url fallback
  std::wstring error;              // for manual-check messaging
};

// Fetch GitHub releases/latest (or, with includePrereleases, the newest non-draft
// release including pre-releases) and compare to kAppVersion. Blocking; call off
// the UI thread.
UpdateCheckResult CheckForUpdates(bool includePrereleases = false);

// True when a is a newer semver than b (strips optional leading 'v').
bool IsVersionNewer(std::wstring_view newerCandidate, std::wstring_view current);

// True if lastUpdateCheckUtc is 0 or older than 24 hours.
bool IsUpdateCheckDue(int64_t lastUpdateCheckUtc);

// Automatic checks: the first runs a while after startup (after the taskbar hook,
// never competing with it), then the host re-evaluates hourly and only goes to
// the network once IsUpdateCheckDue says a day has passed.
inline constexpr unsigned kAutoUpdateFirstDelayMs = 2u * 60u * 1000u;
inline constexpr unsigned kAutoUpdateRetryMs = 60u * 1000u;  // hook still running
inline constexpr unsigned kAutoUpdateTickMs = 60u * 60u * 1000u;

// One notification per found version: due when a pending tag has not been
// announced yet (a notification Windows suppressed is retried on a later tick).
bool UpdateNoticeDue(std::wstring_view pendingTag, std::wstring_view notifiedTag);

int64_t UtcNowUnixSeconds();

}  // namespace phostide
