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

bool ParseBool(const std::string& json, const char* key, bool fallback) {
  const std::string needle = std::string("\"") + key + "\"";
  const auto pos = json.find(needle);
  if (pos == std::string::npos) return fallback;
  const auto colon = json.find(':', pos);
  if (colon == std::string::npos) return fallback;
  const auto rest = json.substr(colon + 1);
  if (rest.find("true") != std::string::npos) return true;
  if (rest.find("false") != std::string::npos) return false;
  return fallback;
}

}  // namespace

std::wstring SettingsDirectory() {
  wchar_t* path = nullptr;
  if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
    return L".";
  }
  std::wstring dir = Join(path, L"Phostide");
  CoTaskMemFree(path);
  CreateDirectoryW(dir.c_str(), nullptr);
  return dir;
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
      << "  \"startWithWindows\": " << (settings.startWithWindows ? "true" : "false") << ",\n"
      << "  \"pauseWhenLocked\": " << (settings.pauseWhenLocked ? "true" : "false") << "\n"
      << "}\n";
  SetStartWithWindows(settings.startWithWindows);
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
