#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace phostide {

struct UpdateCheckResult {
  bool ok = false;                 // network + parse succeeded
  bool updateAvailable = false;    // latest tag newer than kAppVersion
  std::wstring latestTag;          // e.g. "v0.1.3"
  std::wstring setupUrl;           // Setup asset URL or release page
  std::wstring releasePageUrl;     // html_url fallback
  std::wstring error;              // for manual-check messaging
};

// Fetch GitHub releases/latest and compare to kAppVersion. Blocking; call off UI thread.
UpdateCheckResult CheckForUpdates();

// True when a is a newer semver than b (strips optional leading 'v').
bool IsVersionNewer(std::wstring_view newerCandidate, std::wstring_view current);

// True if lastUpdateCheckUtc is 0 or older than 24 hours.
bool IsUpdateCheckDue(int64_t lastUpdateCheckUtc);

int64_t UtcNowUnixSeconds();

}  // namespace phostide
