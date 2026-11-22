// Covers the pure helpers behind Settings > System > Copy diagnostics: log
// decoding, tail trimming and user-path redaction (the text goes into public issues).
#include "../Common/MonitorEnum.cpp"
#include "../Host/Diagnostics.cpp"
#include "TestHarness.h"

using namespace phostide;

namespace {

std::string Utf16Bytes(const std::wstring& s, bool bom) {
  std::string out;
  if (bom) out += "\xFF\xFE";
  out.append(reinterpret_cast<const char*>(s.data()), s.size() * sizeof(wchar_t));
  return out;
}

}  // namespace

TEST(DecodeLogBytes_Utf16WithoutBom) {
  // inject.log / tap.log are raw WriteFile of wchar_t text.
  CHECK(DecodeLogBytes(Utf16Bytes(L"12:00:01 hook ok\r\n", false)) == L"12:00:01 hook ok\r\n");
}

TEST(DecodeLogBytes_Utf16WithBom) {
  CHECK(DecodeLogBytes(Utf16Bytes(L"ready", true)) == L"ready");
}

TEST(DecodeLogBytes_Utf8) {
  CHECK(DecodeLogBytes("0.1.36\r\n") == L"0.1.36\r\n");
  CHECK(DecodeLogBytes("\xEF\xBB\xBF" "abc") == L"abc");
  CHECK(DecodeLogBytes("caf\xC3\xA9") == L"caf\u00E9");
  CHECK(DecodeLogBytes("").empty());
}

TEST(LastLines_KeepsTail) {
  const std::wstring text = L"a\r\nb\r\nc\r\nd\r\n";
  CHECK(LastLines(text, 2) == L"c\r\nd");
  CHECK(LastLines(text, 10) == L"a\r\nb\r\nc\r\nd");
  CHECK(LastLines(text, 0).empty());
  CHECK(LastLines(L"only", 3) == L"only");
}

TEST(RedactUserPaths_CaseInsensitive) {
  const std::wstring profile = L"C:\\Users\\Jane";
  const std::wstring text =
      L"exe=C:\\Users\\Jane\\AppData\\Local\\Programs\\Phostide\\Phostide.exe\r\n"
      L"tap=c:\\users\\jane\\AppData\\Local\\Phostide\\tap\\ExplorerTAP-0.1.36.dll";
  const std::wstring out = RedactUserPaths(text, profile);
  CHECK(out.find(L"Jane") == std::wstring::npos);
  CHECK(out.find(L"jane") == std::wstring::npos);
  CHECK(out.find(L"%USERPROFILE%\\AppData\\Local\\Programs") != std::wstring::npos);
  CHECK(out.find(L"%USERPROFILE%\\AppData\\Local\\Phostide\\tap") != std::wstring::npos);
}

TEST(RedactUserPaths_IgnoresTinyProfile) {
  CHECK(RedactUserPaths(L"C:\\x", L"C:") == L"C:\\x");
}

TEST(BuildDiagnosticsReport_HasSectionsAndNoProfile) {
  Settings s;
  DiagnosticsHostState state;
  state.pauseText = L"Paused until 15:42";
  const std::wstring report = BuildDiagnosticsReport(s, state);
  CHECK(report.find(L"Phostide diagnostics") == 0);
  CHECK(report.find(L"--- Displays ---") != std::wstring::npos);
  CHECK(report.find(L"--- inject.log (last lines) ---") != std::wstring::npos);
  CHECK(report.find(L"Paused until 15:42") != std::wstring::npos);
  wchar_t profile[MAX_PATH]{};
  if (GetEnvironmentVariableW(L"USERPROFILE", profile, MAX_PATH) > 3) {
    CHECK(report.find(profile) == std::wstring::npos);
  }
}
