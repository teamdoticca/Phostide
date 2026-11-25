#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace phostide {

// Only Setup assets from this repo's GitHub releases are downloaded in-app;
// anything else falls back to opening the page in the browser.
bool IsTrustedSetupUrl(std::wstring_view url);

// "Phostide-Setup-0.1.37-win-x64.exe" from a release download URL, or empty when
// the last path segment is not a plain *.exe file name.
std::wstring SetupFileNameFromUrl(std::wstring_view url);

// Signature rule for a downloaded Setup: a signed host requires the exact same
// signer; an unsigned dev host requires the Doticca publisher.
bool IsAcceptedSetupSigner(std::wstring_view setupSigner, std::wstring_view hostSigner);

// True when the exe lives in a Scoop app dir: <scoop root>\apps\phostide\<version or
// current>\Phostide.exe (user or global root). Such installs update with
// `scoop update phostide`; running Setup there would add a second, separate copy.
bool IsScoopInstallPath(std::wstring_view exePath);

struct SetupDownloadResult {
  bool ok = false;
  bool fromCache = false;  // an earlier (background) download was reused
  std::wstring setupPath;
  std::wstring error;  // short, user-facing
};

// percent: 0-100, or -1 while the size is unknown.
using DownloadProgressFn = void (*)(void* context, int percent);

// Verified Setup for url in %TEMP%\Phostide\update. Reuses a file already there
// (background pre-download, or one from before a restart) when it still passes the
// checks, else downloads it again. Checks: Authenticode signer, and a file version
// newer than this build. Blocking; call off the UI thread.
SetupDownloadResult DownloadAndVerifySetup(const std::wstring& url,
                                           DownloadProgressFn progress, void* context);

// Delete downloaded Setups (nothing is pending any more). A Setup that is still
// running is locked and simply stays.
void ClearUpdateDownloads();

// Start the verified Setup silently. It closes Phostide (if still running),
// installs, restarts Explorer when the TAP changed, then starts Phostide again.
bool LaunchSetupForUpdate(const std::wstring& setupPath, std::wstring* error);

}  // namespace phostide
