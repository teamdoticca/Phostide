#include "Diagnostics.h"
#include "../Common/MonitorEnum.h"
#include "MonitorNames.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <vector>

namespace phostide {
namespace {

constexpr size_t kLogTailBytes = 48 * 1024;
constexpr size_t kLogTailLines = 60;

std::wstring TempPhostideDir() {
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  std::wstring dir = temp;
  if (!dir.empty() && dir.back() != L'\\') dir += L'\\';
  return dir + L"Phostide\\";
}

std::wstring EnvVar(const wchar_t* name) {
  wchar_t buf[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(name, buf, MAX_PATH);
  return (n > 0 && n < MAX_PATH) ? std::wstring(buf, n) : std::wstring();
}

// Last kLogTailBytes of a file (aligned to UTF-16 code units).
std::string ReadFileTail(const std::wstring& path) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) return {};
  const std::streamoff size = f.tellg();
  if (size <= 0) return {};
  std::streamoff start = size > static_cast<std::streamoff>(kLogTailBytes)
                             ? size - static_cast<std::streamoff>(kLogTailBytes)
                             : 0;
  start &= ~static_cast<std::streamoff>(1);
  f.seekg(start);
  std::string bytes(static_cast<size_t>(size - start), '\0');
  f.read(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  bytes.resize(static_cast<size_t>(f.gcount()));
  return bytes;
}

std::wstring ReadSmallTextFile(const std::wstring& path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) return {};
  std::string bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  std::wstring text = DecodeLogBytes(bytes);
  while (!text.empty() && (text.back() == L'\r' || text.back() == L'\n' || text.back() == L' ')) {
    text.pop_back();
  }
  return text;
}

std::wstring WindowsVersionText() {
  using RtlGetVersionFn = LONG(WINAPI*)(PRTL_OSVERSIONINFOW);
  RTL_OSVERSIONINFOW vi{sizeof(vi)};
  if (HMODULE ntdll = GetModuleHandleW(L"ntdll.dll")) {
    if (auto fn = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"))) {
      fn(&vi);
    }
  }
  DWORD ubr = 0;
  DWORD cb = sizeof(ubr);
  RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", L"UBR",
               RRF_RT_REG_DWORD, nullptr, &ubr, &cb);
  wchar_t display[64]{};
  DWORD cbDisplay = sizeof(display);
  RegGetValueW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
               L"DisplayVersion", RRF_RT_REG_SZ, nullptr, display, &cbDisplay);
  wchar_t buf[128];
  swprintf_s(buf, L"%lu.%lu.%lu.%lu%s%s", vi.dwMajorVersion, vi.dwMinorVersion, vi.dwBuildNumber,
             ubr, display[0] ? L" " : L"", display);
  return buf;
}

bool NamedEventExists(std::wstring_view name, bool* signaled = nullptr) {
  HANDLE ev = OpenEventW(SYNCHRONIZE, FALSE, name.data());
  if (!ev) return false;
  if (signaled) *signaled = WaitForSingleObject(ev, 0) == WAIT_OBJECT_0;
  CloseHandle(ev);
  return true;
}

const wchar_t* YesNo(bool v) { return v ? L"yes" : L"no"; }

void AppendLogSection(std::wstring& out, const wchar_t* name) {
  out += L"\r\n--- ";
  out += name;
  out += L" (last lines) ---\r\n";
  const std::string bytes = ReadFileTail(TempPhostideDir() + name);
  if (bytes.empty()) {
    out += L"(missing or empty)\r\n";
    return;
  }
  std::wstring tail = LastLines(DecodeLogBytes(bytes), kLogTailLines);
  out += tail;
  if (!tail.empty() && tail.back() != L'\n') out += L"\r\n";
}

}  // namespace

std::wstring RedactUserPaths(std::wstring text, const std::wstring& profileDir) {
  if (profileDir.size() < 4) return text;
  const std::wstring token = L"%USERPROFILE%";
  std::wstring lowerText = text;
  std::wstring lowerProfile = profileDir;
  std::transform(lowerText.begin(), lowerText.end(), lowerText.begin(), ::towlower);
  std::transform(lowerProfile.begin(), lowerProfile.end(), lowerProfile.begin(), ::towlower);
  std::wstring out;
  out.reserve(text.size());
  size_t pos = 0;
  while (true) {
    const size_t hit = lowerText.find(lowerProfile, pos);
    if (hit == std::wstring::npos) break;
    out.append(text, pos, hit - pos);
    out += token;
    pos = hit + profileDir.size();
  }
  out.append(text, pos, std::wstring::npos);
  return out;
}

std::wstring DecodeLogBytes(const std::string& bytes) {
  if (bytes.empty()) return {};
  size_t offset = 0;
  bool utf16 = false;
  if (bytes.size() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFF &&
      static_cast<unsigned char>(bytes[1]) == 0xFE) {
    utf16 = true;
    offset = 2;
  } else if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF &&
             static_cast<unsigned char>(bytes[1]) == 0xBB &&
             static_cast<unsigned char>(bytes[2]) == 0xBF) {
    offset = 3;
  } else {
    // Heuristic: ASCII-heavy UTF-16LE has a zero in most odd bytes.
    size_t pairs = 0;
    size_t zeroHigh = 0;
    for (size_t i = 1; i < bytes.size() && pairs < 512; i += 2, ++pairs) {
      if (bytes[i] == 0) ++zeroHigh;
    }
    utf16 = pairs > 0 && zeroHigh * 2 > pairs;
  }

  if (utf16) {
    const size_t units = (bytes.size() - offset) / 2;
    std::wstring out(units, L'\0');
    memcpy(out.data(), bytes.data() + offset, units * 2);
    return out;
  }
  const int len = static_cast<int>(bytes.size() - offset);
  const int n = MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset, len, nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, bytes.data() + offset, len, out.data(), n);
  return out;
}

std::wstring LastLines(const std::wstring& text, size_t maxLines) {
  if (maxLines == 0 || text.empty()) return {};
  size_t end = text.size();
  // Ignore trailing line breaks when counting.
  while (end > 0 && (text[end - 1] == L'\n' || text[end - 1] == L'\r')) --end;
  size_t pos = end;
  size_t lines = 0;
  while (pos > 0) {
    if (text[pos - 1] == L'\n') {
      if (++lines == maxLines) break;
    }
    --pos;
  }
  return text.substr(pos, end - pos);
}

std::wstring BuildDiagnosticsReport(const Settings& s, const DiagnosticsHostState& state) {
  std::wstring out;
  wchar_t line[512];

  out += L"Phostide diagnostics\r\n";
  swprintf_s(line, L"Version: %.*s\r\n", static_cast<int>(kAppVersion.size()), kAppVersion.data());
  out += line;
  wchar_t exe[MAX_PATH]{};
  GetModuleFileNameW(nullptr, exe, MAX_PATH);
  out += L"Executable: ";
  out += exe;
  out += L"\r\nWindows: " + WindowsVersionText() + L"\r\n";
  SYSTEMTIME st{};
  GetLocalTime(&st);
  swprintf_s(line, L"Local time: %04u-%02u-%02u %02u:%02u\r\n", st.wYear, st.wMonth, st.wDay,
             st.wHour, st.wMinute);
  out += line;

  out += L"\r\n--- State ---\r\n";
  swprintf_s(line, L"Enabled: %s%s%s\r\n", YesNo(s.enabled || !state.pauseText.empty()),
             state.pauseText.empty() ? L"" : L", ", state.pauseText.c_str());
  out += line;
  bool boundSignaled = false;
  const bool tapLoaded = NamedEventExists(kTapReadyEvent);
  NamedEventExists(kTapBoundEvent, &boundSignaled);
  swprintf_s(line, L"TAP loaded in Explorer: %s; bound: %s (host view: %s)\r\n", YesNo(tapLoaded),
             YesNo(boundSignaled), YesNo(state.tapBound));
  out += line;
  const std::wstring stamp = ReadSmallTextFile(TempPhostideDir() + L"tap.version");
  out += L"TAP version stamp: " + (stamp.empty() ? std::wstring(L"(none)") : stamp) + L"\r\n";
  swprintf_s(line, L"Session locked: %s; fullscreen pause: %s\r\n", YesNo(state.sessionLocked),
             YesNo(state.fullscreenPaused));
  out += line;

  out += L"\r\n--- Settings ---\r\n";
  swprintf_s(line,
             L"Render: %s; start with Windows: %s; auto update check: %s; "
             L"pre-release updates: %s\r\n",
             RenderModeName(s.renderMode), YesNo(s.startWithWindows),
             YesNo(s.autoCheckUpdates), YesNo(s.updateIncludePrereleases));
  out += line;
  swprintf_s(line, L"Fallback look: %s %d%%\r\n", ThemeName(s.theme), s.intensity);
  out += line;
  swprintf_s(line,
             L"OLED Care: idle %d min, idle fill %d%%, chrome dim %s, clock nudge %s, "
             L"night boost %s\r\n",
             s.oledCareIdleMinutes, s.oledCareFillIntensity, YesNo(s.oledCareChromeDim),
             YesNo(s.oledCareClockNudge), YesNo(s.oledCareNightBoost));
  out += line;
  swprintf_s(line, L"Pause when locked: %s; pause in fullscreen: %s\r\n", YesNo(s.pauseWhenLocked),
             YesNo(s.pauseWhenFullscreen));
  out += line;

  out += L"\r\n--- Displays ---\r\n";
  const auto monitors = EnumerateAttachedMonitors();
  const auto modelNames = QueryMonitorFriendlyNames();
  int index = 1;
  for (const auto& mon : monitors) {
    const MonitorAppearance a = ResolveMonitorAppearance(s, mon.deviceName);
    const auto model = modelNames.find(mon.deviceName);
    const std::wstring modelText =
        model != modelNames.end() ? L" (" + model->second + L")" : std::wstring();
    swprintf_s(line, L"%d. %s%s %dx%d%s - %s, %s %d%%\r\n", index++, mon.deviceName.c_str(),
               modelText.c_str(), mon.width, mon.height, mon.primary ? L" primary" : L"",
               IsMonitorDisabled(s, mon.deviceName) ? L"off" : L"on", ThemeName(a.theme),
               a.intensity);
    out += line;
  }
  if (monitors.empty()) out += L"(none enumerated)\r\n";

  AppendLogSection(out, L"inject.log");
  AppendLogSection(out, L"tap.log");

  // Strip NULs (a torn UTF-16 write) so the clipboard text is not cut short.
  out.erase(std::remove(out.begin(), out.end(), L'\0'), out.end());
  // %TEMP% (and so the log paths) is often the 8.3 form, e.g. C:\Users\JOHNDO~1.
  const std::wstring profile = EnvVar(L"USERPROFILE");
  wchar_t shortProfile[MAX_PATH]{};
  const DWORD shortLen = GetShortPathNameW(profile.c_str(), shortProfile, MAX_PATH);
  out = RedactUserPaths(std::move(out), profile);
  if (shortLen > 0 && shortLen < MAX_PATH) out = RedactUserPaths(std::move(out), shortProfile);
  return out;
}

bool CopyTextToClipboard(HWND owner, const std::wstring& text) {
  if (!OpenClipboard(owner)) return false;
  bool ok = false;
  if (EmptyClipboard()) {
    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    if (HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes)) {
      if (void* dst = GlobalLock(mem)) {
        memcpy(dst, text.c_str(), bytes);
        GlobalUnlock(mem);
        ok = SetClipboardData(CF_UNICODETEXT, mem) != nullptr;
      }
      if (!ok) GlobalFree(mem);
    }
  }
  CloseClipboard();
  return ok;
}

}  // namespace phostide
