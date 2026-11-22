#include "SettingsStore.h"

#include <fstream>
#include <sstream>

#include <shlobj.h>
#include <windows.h>

namespace phostide {
namespace {

std::wstring Join(const std::wstring& a, const std::wstring& b) {
  if (a.empty()) return b;
  if (a.back() == L'\\' || a.back() == L'/') return a + b;
  return a + L"\\" + b;
}

int ParseInt(const std::string& json, const char* key, int fallback) {
  const std::string needle = std::string("\"") + key + "\"";
  const auto pos = json.find(needle);
  if (pos == std::string::npos) return fallback;
  const auto colon = json.find(':', pos);
  if (colon == std::string::npos) return fallback;
  return std::atoi(json.c_str() + colon + 1);
}

int64_t ParseInt64(const std::string& json, const char* key, int64_t fallback) {
  const std::string needle = std::string("\"") + key + "\"";
  const auto pos = json.find(needle);
  if (pos == std::string::npos) return fallback;
  const auto colon = json.find(':', pos);
  if (colon == std::string::npos) return fallback;
  return static_cast<int64_t>(_atoi64(json.c_str() + colon + 1));
}

bool ParseBool(const std::string& json, const char* key, bool fallback) {
  const std::string needle = std::string("\"") + key + "\"";
  const auto pos = json.find(needle);
  if (pos == std::string::npos) return fallback;
  const auto colon = json.find(':', pos);
  if (colon == std::string::npos) return fallback;
  // Read only this key's value. Searching the rest of the file let any later
  // `true` flip an earlier `false` (enabled, pause toggles, chrome dim reset on
  // every restart).
  size_t i = colon + 1;
  while (i < json.size() &&
         (json[i] == ' ' || json[i] == '\t' || json[i] == '\r' || json[i] == '\n')) {
    ++i;
  }
  if (json.compare(i, 4, "true") == 0) return true;
  if (json.compare(i, 5, "false") == 0) return false;
  return fallback;
}

// PHOSTIDE_SETTINGS_DIR = tests / isolated dev runs. Those must never rewrite or
// delete the user's real autostart entry (unit tests used to remove it).
bool IsolatedSettingsDir() {
  wchar_t buf[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(L"PHOSTIDE_SETTINGS_DIR", buf, MAX_PATH);
  return n > 0 && n < MAX_PATH;
}

std::wstring Utf8ToWide(const std::string& utf8) {
  if (utf8.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                      out.data(), n);
  return out;
}

std::string WideToUtf8(const std::wstring& wide) {
  if (wide.empty()) return {};
  const int n = WideCharToMultiByte(CP_UTF8, 0, wide.data(),
                                    static_cast<int>(wide.size()), nullptr, 0,
                                    nullptr, nullptr);
  if (n <= 0) return {};
  std::string out(static_cast<size_t>(n), '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                      out.data(), n, nullptr, nullptr);
  return out;
}

std::string ParseUtf8String(const std::string& json, const char* key) {
  const std::string needle = std::string("\"") + key + "\"";
  const auto pos = json.find(needle);
  if (pos == std::string::npos) return {};
  const auto colon = json.find(':', pos);
  if (colon == std::string::npos) return {};
  size_t i = colon + 1;
  while (i < json.size() &&
         (json[i] == ' ' || json[i] == '\t' || json[i] == '\r' || json[i] == '\n')) {
    ++i;
  }
  if (i >= json.size() || json[i] != '"') return {};
  ++i;
  std::string out;
  while (i < json.size()) {
    const char c = json[i++];
    if (c == '\\' && i < json.size()) {
      const char e = json[i++];
      if (e == '"' || e == '\\' || e == '/') {
        out.push_back(e);
      } else if (e == 'n') {
        out.push_back('\n');
      } else if (e == 't') {
        out.push_back('\t');
      } else {
        out.push_back(e);
      }
    } else if (c == '"') {
      return out;
    } else {
      out.push_back(c);
    }
  }
  return {};
}

std::string EscapeJson(const std::string& s) {
  std::string out;
  out.reserve(s.size() + 8);
  for (const char c : s) {
    switch (c) {
      case '"':
        out += "\\\"";
        break;
      case '\\':
        out += "\\\\";
        break;
      case '\n':
        out += "\\n";
        break;
      case '\r':
        out += "\\r";
        break;
      case '\t':
        out += "\\t";
        break;
      default:
        out.push_back(c);
        break;
    }
  }
  return out;
}

std::string ReadJsonStringValue(const std::string& s, size_t& i) {
  while (i < s.size() &&
         (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) {
    ++i;
  }
  if (i >= s.size() || s[i] != '"') return {};
  ++i;
  std::string out;
  while (i < s.size()) {
    const char c = s[i++];
    if (c == '\\' && i < s.size()) {
      out.push_back(s[i++]);
    } else if (c == '"') {
      return out;
    } else {
      out.push_back(c);
    }
  }
  return {};
}

void ParseDisabledMonitors(const std::string& json, Settings& s) {
  const auto pos = json.find("\"disabledMonitors\"");
  if (pos == std::string::npos) return;
  const auto bracket = json.find('[', pos);
  if (bracket == std::string::npos) return;
  const auto end = json.find(']', bracket);
  if (end == std::string::npos) return;
  size_t i = bracket + 1;
  while (i < end) {
    const std::string name = ReadJsonStringValue(json, i);
    if (!name.empty()) s.disabledMonitors.push_back(Utf8ToWide(name));
    while (i < end && (json[i] == ',' || json[i] == ' ' || json[i] == '\n' ||
                       json[i] == '\r' || json[i] == '\t')) {
      ++i;
    }
  }
}

void ParseMonitorSettings(const std::string& json, Settings& s) {
  const auto pos = json.find("\"monitorSettings\"");
  if (pos == std::string::npos) return;
  const auto brace = json.find('{', pos);
  if (brace == std::string::npos) return;
  int depth = 0;
  size_t end = brace;
  for (; end < json.size(); ++end) {
    if (json[end] == '{') ++depth;
    else if (json[end] == '}') {
      --depth;
      if (depth == 0) break;
    }
  }
  if (depth != 0) return;
  size_t i = brace + 1;
  while (i < end) {
    while (i < end && (json[i] == ' ' || json[i] == '\n' || json[i] == '\r' ||
                       json[i] == '\t' || json[i] == ',')) {
      ++i;
    }
    if (i >= end) break;
    const std::string key = ReadJsonStringValue(json, i);
    if (key.empty()) break;
    const auto colon = json.find(':', i);
    if (colon == std::string::npos || colon >= end) break;
    const auto objStart = json.find('{', colon);
    if (objStart == std::string::npos || objStart >= end) break;
    const auto objEnd = json.find('}', objStart);
    if (objEnd == std::string::npos || objEnd > end) break;
    const std::string obj = json.substr(objStart, objEnd - objStart + 1);
    MonitorAppearanceEntry entry;
    entry.deviceName = Utf8ToWide(key);
    entry.appearance.theme = ThemeFromIndex(ParseInt(obj, "theme", static_cast<int>(s.theme)));
    entry.appearance.intensity = ParseInt(obj, "intensity", s.intensity);
    // Legacy per-monitor renderMode keys are ignored — rendering is app-global.
    s.monitorSettings.push_back(std::move(entry));
    i = objEnd + 1;
  }
}

}  // namespace

std::wstring SettingsDirectory() {
  // Override for tests and isolated dev runs (e.g. first-run Welcome checks)
  // without touching the user's real %APPDATA%\Phostide.
  wchar_t overrideDir[MAX_PATH]{};
  const DWORD n = GetEnvironmentVariableW(L"PHOSTIDE_SETTINGS_DIR", overrideDir, MAX_PATH);
  if (n > 0 && n < MAX_PATH) {
    CreateDirectoryW(overrideDir, nullptr);
    return overrideDir;
  }
  wchar_t* path = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
    return L".";
  }
  std::wstring dir = Join(path, L"Phostide");
  CoTaskMemFree(path);
  CreateDirectoryW(dir.c_str(), nullptr);
  return dir;
}

bool SettingsFileExists() {
  const auto file = Join(SettingsDirectory(), std::wstring(kSettingsFileName));
  const DWORD attrs = GetFileAttributesW(file.c_str());
  return attrs != INVALID_FILE_ATTRIBUTES && !(attrs & FILE_ATTRIBUTE_DIRECTORY);
}

Settings LoadSettings() {
  Settings s;
  const auto file = Join(SettingsDirectory(), std::wstring(kSettingsFileName));
  std::ifstream in(file);
  if (!in) {
    s.startWithWindows = IsStartWithWindowsEnabled();
    return s;
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  const std::string json = ss.str();
  s.enabled = ParseBool(json, "enabled", true);
  s.theme = ThemeFromIndex(ParseInt(json, "theme", 0));
  s.intensity = ParseInt(json, "intensity", 45);
  s.startWithWindows = ParseBool(json, "startWithWindows", IsStartWithWindowsEnabled());
  s.pauseWhenLocked = ParseBool(json, "pauseWhenLocked", true);
  s.pauseWhenFullscreen = ParseBool(json, "pauseWhenFullscreen", true);
  s.renderMode = RenderModeFromIndex(ParseInt(json, "renderMode", 0));
  s.oledCareIdleMinutes =
      ClampOledCareIdleMinutes(ParseInt(json, "oledCareIdleMinutes", 0));
  s.oledCareFillIntensity = ClampPercent(ParseInt(json, "oledCareFillIntensity", 8));
  s.oledCareChromeDim = ParseBool(json, "oledCareChromeDim", false);
  s.oledCareChromeOpacity = ClampPercent(ParseInt(json, "oledCareChromeOpacity", 35));
  s.oledCareClockOpacity = ClampPercent(ParseInt(json, "oledCareClockOpacity", 22));
  s.oledCareClockNudge = ParseBool(json, "oledCareClockNudge", false);
  s.oledCareNightBoost = ParseBool(json, "oledCareNightBoost", false);
  s.lastUpdateCheckUtc = ParseInt64(json, "lastUpdateCheckUtc", 0);
  s.pendingUpdateTag = Utf8ToWide(ParseUtf8String(json, "pendingUpdateTag"));
  s.pendingSetupUrl = Utf8ToWide(ParseUtf8String(json, "pendingSetupUrl"));
  ParseDisabledMonitors(json, s);
  ParseMonitorSettings(json, s);
  return s;
}

void SaveSettings(const Settings& settings) {
  const auto file = Join(SettingsDirectory(), std::wstring(kSettingsFileName));
  std::ofstream out(file, std::ios::trunc);
  if (!out) return;
  out << "{\n"
      << "  \"enabled\": " << (settings.enabled ? "true" : "false") << ",\n"
      << "  \"theme\": " << static_cast<int>(settings.theme) << ",\n"
      << "  \"intensity\": " << settings.intensity << ",\n"
      << "  \"startWithWindows\": " << (settings.startWithWindows ? "true" : "false")
      << ",\n"
      << "  \"pauseWhenLocked\": " << (settings.pauseWhenLocked ? "true" : "false")
      << ",\n"
      << "  \"pauseWhenFullscreen\": "
      << (settings.pauseWhenFullscreen ? "true" : "false") << ",\n"
      << "  \"renderMode\": " << static_cast<int>(settings.renderMode) << ",\n"
      << "  \"oledCareIdleMinutes\": " << settings.oledCareIdleMinutes << ",\n"
      << "  \"oledCareFillIntensity\": " << settings.oledCareFillIntensity << ",\n"
      << "  \"oledCareChromeDim\": " << (settings.oledCareChromeDim ? "true" : "false")
      << ",\n"
      << "  \"oledCareChromeOpacity\": " << settings.oledCareChromeOpacity << ",\n"
      << "  \"oledCareClockOpacity\": " << settings.oledCareClockOpacity << ",\n"
      << "  \"oledCareClockNudge\": " << (settings.oledCareClockNudge ? "true" : "false")
      << ",\n"
      << "  \"oledCareNightBoost\": " << (settings.oledCareNightBoost ? "true" : "false")
      << ",\n"
      << "  \"lastUpdateCheckUtc\": " << settings.lastUpdateCheckUtc << ",\n"
      << "  \"pendingUpdateTag\": \""
      << EscapeJson(WideToUtf8(settings.pendingUpdateTag)) << "\",\n"
      << "  \"pendingSetupUrl\": \""
      << EscapeJson(WideToUtf8(settings.pendingSetupUrl)) << "\",\n"
      << "  \"disabledMonitors\": [";
  for (size_t i = 0; i < settings.disabledMonitors.size(); ++i) {
    if (i) out << ", ";
    out << "\"" << EscapeJson(WideToUtf8(settings.disabledMonitors[i])) << "\"";
  }
  out << "],\n"
      << "  \"monitorSettings\": {\n";
  for (size_t i = 0; i < settings.monitorSettings.size(); ++i) {
    const auto& e = settings.monitorSettings[i];
    if (i) out << ",\n";
    out << "    \"" << EscapeJson(WideToUtf8(e.deviceName)) << "\": {"
        << "\"theme\": " << static_cast<int>(e.appearance.theme) << ", "
        << "\"intensity\": " << e.appearance.intensity << "}";
  }
  if (!settings.monitorSettings.empty()) out << "\n";
  out << "  }\n"
      << "}\n";
  if (!IsolatedSettingsDir()) SetStartWithWindows(settings.startWithWindows);
}

void SetStartWithWindows(bool enabled) {
  HKEY key = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, nullptr, 0,
                      KEY_SET_VALUE | KEY_QUERY_VALUE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
    return;
  }
  if (enabled) {
    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    RegSetValueExW(key, L"Phostide", 0, REG_SZ, reinterpret_cast<const BYTE*>(exe),
                   static_cast<DWORD>((wcslen(exe) + 1) * sizeof(wchar_t)));
  } else {
    RegDeleteValueW(key, L"Phostide");
  }
  RegCloseKey(key);
}

bool IsStartWithWindowsEnabled() {
  HKEY key = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                    KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
    return false;
  }
  DWORD type = 0;
  const LSTATUS st = RegQueryValueExW(key, L"Phostide", nullptr, &type, nullptr, nullptr);
  RegCloseKey(key);
  return st == ERROR_SUCCESS;
}

}  // namespace phostide
