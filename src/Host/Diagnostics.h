#pragma once

#include "../Common/PhostideCommon.h"

#include <string>

#include <windows.h>

namespace phostide {

// Live host state that is not in Settings (for the diagnostics report).
struct DiagnosticsHostState {
  std::wstring pauseText;  // e.g. "paused until 15:42"; empty when not temp-paused
  bool sessionLocked = false;
  bool fullscreenPaused = false;
  bool tapBound = false;
};

// Plain-text report for bug reports: version, Windows build, displays, settings,
// TAP state and the tail of inject.log / tap.log. The user profile path is
// replaced with %USERPROFILE% so the text is safe to paste into a public issue.
std::wstring BuildDiagnosticsReport(const Settings& settings, const DiagnosticsHostState& state);

bool CopyTextToClipboard(HWND owner, const std::wstring& text);

// Helpers (exposed for unit tests).
std::wstring RedactUserPaths(std::wstring text, const std::wstring& profileDir);
// Logs are written as UTF-16LE (no BOM); fall back to UTF-8 for anything else.
std::wstring DecodeLogBytes(const std::string& bytes);
std::wstring LastLines(const std::wstring& text, size_t maxLines);

}  // namespace phostide
