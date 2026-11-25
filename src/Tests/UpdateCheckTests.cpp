// Covers version comparison and the hand-rolled GitHub JSON helpers.
#include "../Host/UpdateCheck.cpp"
#include "TestHarness.h"

using namespace phostide;

TEST(VersionNewer_ComparesSemverParts) {
  CHECK(IsVersionNewer(L"v0.1.33", L"0.1.32"));
  CHECK(IsVersionNewer(L"0.2.0", L"0.1.99"));
  CHECK(IsVersionNewer(L"1.0.0", L"0.9.9"));
  CHECK(IsVersionNewer(L"v0.1.10", L"0.1.9"));  // numeric, not lexicographic
  CHECK(!IsVersionNewer(L"v0.1.32", L"0.1.32"));
  CHECK(!IsVersionNewer(L"0.1.31", L"0.1.32"));
  CHECK(!IsVersionNewer(L"V0.1.9", L"0.1.10"));
}

TEST(VersionNewer_RejectsGarbage) {
  CHECK(!IsVersionNewer(L"", L"0.1.32"));
  CHECK(!IsVersionNewer(L"latest", L"0.1.32"));
  CHECK(!IsVersionNewer(L"v", L"0.1.32"));
}

TEST(VersionTriple_ParsesPartialVersions) {
  int a = -1, b = -1, c = -1;
  CHECK(ParseTriple(L"v2", a, b, c) && a == 2 && b == 0 && c == 0);
  CHECK(ParseTriple(L"1.4", a, b, c) && a == 1 && b == 4 && c == 0);
  CHECK(ParseTriple(L"0.1.33-beta", a, b, c) && a == 0 && b == 1 && c == 33);
  CHECK(!ParseTriple(L"x.1.2", a, b, c));
}

TEST(UpdateCheckDue_After24Hours) {
  const int64_t now = UtcNowUnixSeconds();
  CHECK(IsUpdateCheckDue(0));
  CHECK(IsUpdateCheckDue(now - 25 * 3600));
  CHECK(!IsUpdateCheckDue(now - 3600));
}

TEST(JsonString_ReadsValueAndEscapes) {
  const std::string json =
      R"({"html_url": "https://x/rel", "tag_name" : "v0.1.33", "body": "a \"b\"\nc"})";
  CHECK(ExtractJsonString(json, "tag_name") == "v0.1.33");
  CHECK(ExtractJsonString(json, "html_url") == "https://x/rel");
  CHECK(ExtractJsonString(json, "body") == "a \"b\"\nc");
  CHECK(ExtractJsonString(json, "missing").empty());
}

TEST(JsonString_SkipsNonStringMatch) {
  // First "tag_name" is a number; the helper must keep scanning to the string.
  const std::string json = R"({"meta": {"tag_name": 5}, "tag_name": "v1.2.3"})";
  CHECK(ExtractJsonString(json, "tag_name") == "v1.2.3");
}

TEST(SetupUrl_PrefersPhostideSetupExe) {
  const std::string json = R"({"assets": [
    {"browser_download_url": "https://g/d/v0.1.33/Phostide-0.1.33-win-x64.zip"},
    {"browser_download_url": "https://g/d/v0.1.33/Other-Setup.exe"},
    {"browser_download_url": "https://g/d/v0.1.33/Phostide-Setup-0.1.33-win-x64.exe"}]})";
  CHECK(FindSetupDownloadUrl(json) ==
        "https://g/d/v0.1.33/Phostide-Setup-0.1.33-win-x64.exe");
}

TEST(SetupUrl_FallsBackToAnySetupExe_ElseEmpty) {
  CHECK(FindSetupDownloadUrl(R"({"browser_download_url": "https://g/My-SETUP.EXE"})") ==
        "https://g/My-SETUP.EXE");
  CHECK(FindSetupDownloadUrl(R"({"browser_download_url": "https://g/app.zip"})").empty());
}

// Shape of GET /repos/.../releases (newest first, nested author/assets, braces in bodies).
static const std::string kReleaseList = R"([
  {"url": "u", "html_url": "https://github.com/teamdoticca/Phostide/releases/tag/v0.1.43",
   "tag_name": "v0.1.43", "draft": true, "prerelease": false,
   "author": {"login": "x", "html_url": "https://github.com/x"}, "body": "draft {x}"},
  {"url": "u", "html_url": "https://github.com/teamdoticca/Phostide/releases/tag/v0.1.42",
   "tag_name": "v0.1.42", "draft": false, "prerelease": true,
   "author": {"login": "x", "html_url": "https://github.com/x"},
   "assets": [{"name": "Phostide-Setup-0.1.42-win-x64.exe",
     "browser_download_url": "https://github.com/teamdoticca/Phostide/releases/download/v0.1.42/Phostide-Setup-0.1.42-win-x64.exe"}],
   "body": "fix \"quotes\" and } braces"},
  {"url": "u", "html_url": "https://github.com/teamdoticca/Phostide/releases/tag/v0.1.41",
   "tag_name": "v0.1.41", "draft": false, "prerelease": false, "assets": []}
])";

TEST(SelectNewestRelease_PicksNewestNonDraftIncludingPrerelease) {
  const std::string pick = SelectNewestRelease(kReleaseList);
  CHECK(ExtractJsonString(pick, "tag_name") == "v0.1.42");
  CHECK(ExtractJsonBool(pick, "prerelease", false));
  CHECK(ExtractJsonString(pick, "html_url") ==
        "https://github.com/teamdoticca/Phostide/releases/tag/v0.1.42");
  CHECK(FindSetupDownloadUrl(pick).find("Phostide-Setup-0.1.42") != std::string::npos);
}

TEST(SelectNewestRelease_StableWinsWhenNewer) {
  // Pre-release opt-in never offers an older pre-release over a newer stable.
  const std::string list = R"([
    {"tag_name": "v0.2.0", "draft": false, "prerelease": false},
    {"tag_name": "v0.1.99", "draft": false, "prerelease": true}])";
  CHECK(ExtractJsonString(SelectNewestRelease(list), "tag_name") == "v0.2.0");
}

TEST(SelectNewestRelease_EmptyOrUnparseable) {
  CHECK(SelectNewestRelease("[]").empty());
  CHECK(SelectNewestRelease("").empty());
  CHECK(SelectNewestRelease(R"([{"tag_name": "nightly", "draft": false}])").empty());
}

TEST(UpdateNotice_OncePerVersion) {
  CHECK(UpdateNoticeDue(L"v0.1.42", L""));
  CHECK(UpdateNoticeDue(L"v0.1.43", L"v0.1.42"));  // a newer one is announced again
  CHECK(!UpdateNoticeDue(L"v0.1.42", L"v0.1.42"));
  CHECK(!UpdateNoticeDue(L"", L"v0.1.42"));         // nothing pending
}

TEST(AutoUpdateSchedule_FirstCheckWaitsAndTicksHourly) {
  CHECK(kAutoUpdateFirstDelayMs >= 60u * 1000u);  // never at startup
  CHECK(kAutoUpdateTickMs <= 24u * 60u * 60u * 1000u);
  CHECK(IsUpdateCheckDue(0));
  CHECK(!IsUpdateCheckDue(UtcNowUnixSeconds() - 60));
  CHECK(IsUpdateCheckDue(UtcNowUnixSeconds() - 25 * 60 * 60));
}
