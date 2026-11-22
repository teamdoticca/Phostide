// Covers settings.json parsing and a Save -> Load round trip in a temp folder
// (PHOSTIDE_SETTINGS_DIR), never the user's real %APPDATA%\Phostide.
#include "../Host/SettingsStore.cpp"
#include "TestHarness.h"

#include <filesystem>

using namespace phostide;

namespace {

std::wstring UseTempSettingsDir(const wchar_t* leaf) {
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  const std::filesystem::path dir = std::filesystem::path(temp) / L"PhostideTests" / leaf;
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
  SetEnvironmentVariableW(L"PHOSTIDE_SETTINGS_DIR", dir.c_str());
  return dir.wstring();
}

}  // namespace

TEST(ParseBool_ReadsOnlyItsOwnValue) {
  // Regression: a later `true` must not flip an earlier `false`.
  const std::string json =
      "{\n  \"enabled\": false,\n  \"pauseWhenLocked\": true,\n"
      "  \"oledCareChromeDim\": false,\n  \"oledCareNightBoost\": true\n}";
  CHECK(ParseBool(json, "enabled", true) == false);
  CHECK(ParseBool(json, "oledCareChromeDim", true) == false);
  CHECK(ParseBool(json, "pauseWhenLocked", false) == true);
  CHECK(ParseBool(json, "oledCareNightBoost", false) == true);
  CHECK(ParseBool(json, "missing", true) == true);
  CHECK(ParseBool("{\"enabled\": 1}", "enabled", true) == true);  // not a bool -> fallback
}

TEST(ParseInt_ReadsNumbers) {
  const std::string json = R"({"theme": 4, "intensity":35, "neg": -3})";
  CHECK(ParseInt(json, "theme", 0) == 4);
  CHECK(ParseInt(json, "intensity", 0) == 35);
  CHECK(ParseInt(json, "neg", 0) == -3);
  CHECK(ParseInt(json, "missing", 7) == 7);
}

TEST(EscapeJson_RoundTripsThroughStringParser) {
  const std::string raw = "C:\\Users\\\"x\"\n";
  const std::string json = "{\"k\": \"" + EscapeJson(raw) + "\"}";
  CHECK(ParseUtf8String(json, "k") == raw);
}

TEST(MonitorLists_ParseDeviceNamesWithBackslashes) {
  Settings s;
  const std::string json =
      R"({"theme": 3, "intensity": 20, "disabledMonitors": ["\\\\.\\DISPLAY2"],)"
      R"( "monitorSettings": {"\\\\.\\DISPLAY1": {"theme": 1, "intensity": 60},)"
      R"( "\\\\.\\DISPLAY2": {"intensity": 10}}})";
  s.theme = ThemeId::Space;
  s.intensity = 20;
  ParseDisabledMonitors(json, s);
  ParseMonitorSettings(json, s);
  CHECK(s.disabledMonitors.size() == 1 && s.disabledMonitors[0] == L"\\\\.\\DISPLAY2");
  CHECK(s.monitorSettings.size() == 2);
  if (s.monitorSettings.size() == 2) {
    CHECK(s.monitorSettings[0].deviceName == L"\\\\.\\DISPLAY1");
    CHECK(s.monitorSettings[0].appearance.theme == ThemeId::SeaWaves);
    CHECK(s.monitorSettings[0].appearance.intensity == 60);
    // Missing per-monitor theme falls back to the top-level theme.
    CHECK(s.monitorSettings[1].appearance.theme == ThemeId::Space);
    CHECK(s.monitorSettings[1].appearance.intensity == 10);
  }
}

TEST(FreshInstall_UsesOledFirstDefaults) {
  UseTempSettingsDir(L"fresh");
  CHECK(!SettingsFileExists());
  const Settings s = LoadSettings();
  CHECK(s.theme == ThemeId::Aurora);
  CHECK(s.intensity == 35);
  CHECK(s.oledCareIdleMinutes == 5);
  CHECK(s.enabled);
  CHECK(s.pauseWhenLocked && s.pauseWhenFullscreen);
  CHECK(s.disabledMonitors.empty() && s.monitorSettings.empty());
}

TEST(SaveLoad_RoundTripsEveryField) {
  UseTempSettingsDir(L"roundtrip");
  Settings in;
  in.enabled = false;  // tray soft-pause must survive a restart
  in.theme = ThemeId::Weather;
  in.intensity = 62;
  in.pauseWhenLocked = false;
  in.pauseWhenFullscreen = false;
  in.renderMode = RenderMode::Cpu;
  in.oledCareIdleMinutes = 10;
  in.oledCareFillIntensity = 3;
  in.oledCareChromeDim = false;
  in.oledCareChromeOpacity = 40;
  in.oledCareClockOpacity = 15;
  in.oledCareClockNudge = true;
  in.oledCareNightBoost = false;
  in.lastUpdateCheckUtc = 1794907410;
  in.pendingUpdateTag = L"v0.1.40";
  in.pendingSetupUrl = L"https://example.test/Phostide-Setup.exe";
  in.disabledMonitors = {L"\\\\.\\DISPLAY3"};
  in.monitorSettings.push_back({L"\\\\.\\DISPLAY1", {ThemeId::Space, 30}});
  in.monitorSettings.push_back({L"\\\\.\\DISPLAY3", {ThemeId::Rainbow, 80}});

  SaveSettings(in);
  CHECK(SettingsFileExists());
  const Settings out = LoadSettings();

  CHECK(out.enabled == in.enabled);
  CHECK(out.theme == in.theme);
  CHECK(out.intensity == in.intensity);
  CHECK(out.pauseWhenLocked == in.pauseWhenLocked);
  CHECK(out.pauseWhenFullscreen == in.pauseWhenFullscreen);
  CHECK(out.renderMode == in.renderMode);
  CHECK(out.oledCareIdleMinutes == in.oledCareIdleMinutes);
  CHECK(out.oledCareFillIntensity == in.oledCareFillIntensity);
  CHECK(out.oledCareChromeDim == in.oledCareChromeDim);
  CHECK(out.oledCareChromeOpacity == in.oledCareChromeOpacity);
  CHECK(out.oledCareClockOpacity == in.oledCareClockOpacity);
  CHECK(out.oledCareClockNudge == in.oledCareClockNudge);
  CHECK(out.oledCareNightBoost == in.oledCareNightBoost);
  CHECK(out.lastUpdateCheckUtc == in.lastUpdateCheckUtc);
  CHECK(out.pendingUpdateTag == in.pendingUpdateTag);
  CHECK(out.pendingSetupUrl == in.pendingSetupUrl);
  CHECK(out.disabledMonitors == in.disabledMonitors);
  CHECK(out.monitorSettings.size() == 2);
  if (out.monitorSettings.size() == 2) {
    CHECK(out.monitorSettings[1].deviceName == L"\\\\.\\DISPLAY3");
    CHECK(out.monitorSettings[1].appearance.theme == ThemeId::Rainbow);
    CHECK(out.monitorSettings[1].appearance.intensity == 80);
  }
}

TEST(Load_ClampsOutOfRangeCareValues) {
  const std::wstring dir = UseTempSettingsDir(L"clamp");
  {
    std::ofstream f(std::filesystem::path(dir) / L"settings.json");
    f << R"({"oledCareIdleMinutes": 7, "oledCareFillIntensity": 250, "theme": 99})";
  }
  const Settings s = LoadSettings();
  CHECK(s.oledCareIdleMinutes == 0);  // not one of Off/1/5/10/30
  CHECK(s.oledCareFillIntensity == 100);
  CHECK(static_cast<int>(s.theme) < static_cast<int>(ThemeId::Count));
}

namespace {

// Current HKCU Run\Phostide value, or "<absent>".
std::wstring RunEntry() {
  wchar_t buf[1024]{};
  DWORD cb = sizeof(buf);
  if (RegGetValueW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                   L"Phostide", RRF_RT_REG_SZ, nullptr, buf, &cb) != ERROR_SUCCESS) {
    return L"<absent>";
  }
  return buf;
}

}  // namespace

TEST(Save_WithSettingsDirOverride_LeavesAutostartAlone) {
  // Regression: these tests used to delete the developer's real autostart entry.
  UseTempSettingsDir(L"autostart");
  const std::wstring before = RunEntry();
  Settings s;
  s.startWithWindows = false;
  SaveSettings(s);
  CHECK(RunEntry() == before);
  s.startWithWindows = true;
  SaveSettings(s);
  CHECK(RunEntry() == before);
}
