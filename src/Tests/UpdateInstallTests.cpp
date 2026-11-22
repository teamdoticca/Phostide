// Covers the guards in front of in-app "Update now": which URLs may be downloaded,
// how the local file name is derived, and which Setup signers are accepted.
#include "../Host/Signing.cpp"
#include "../Host/UpdateInstall.cpp"
#include "TestHarness.h"

using namespace phostide;

TEST(IsTrustedSetupUrl_OnlyOurReleaseSetup) {
  CHECK(IsTrustedSetupUrl(
      L"https://github.com/teamdoticca/Phostide/releases/download/v0.1.37/"
      L"Phostide-Setup-0.1.37-win-x64.exe"));
  CHECK(IsTrustedSetupUrl(
      L"HTTPS://GitHub.com/TeamDoticca/Phostide/releases/download/v0.1.37/"
      L"Phostide-Setup-0.1.37-win-x64.exe"));
  // Wrong scheme, repo, asset or a release page.
  CHECK(!IsTrustedSetupUrl(
      L"http://github.com/teamdoticca/Phostide/releases/download/v0.1.37/"
      L"Phostide-Setup-0.1.37-win-x64.exe"));
  CHECK(!IsTrustedSetupUrl(
      L"https://github.com/someone/Phostide/releases/download/v0.1.37/"
      L"Phostide-Setup-0.1.37-win-x64.exe"));
  CHECK(!IsTrustedSetupUrl(
      L"https://github.com/teamdoticca/Phostide/releases/download/v0.1.37/"
      L"Phostide-0.1.37-win-x64.zip"));
  CHECK(!IsTrustedSetupUrl(L"https://github.com/teamdoticca/Phostide/releases/tag/v0.1.37"));
  CHECK(!IsTrustedSetupUrl(
      L"https://github.com/teamdoticca/Phostide/releases/download/v0.1.37/"
      L"Phostide-Setup-0.1.37-win-x64.exe.evil.com/x.exe"));
  CHECK(!IsTrustedSetupUrl(
      L"https://github.com/teamdoticca/Phostide/releases/download/../../../other/repo/"
      L"releases/download/v1/Phostide-Setup-1.0.0-win-x64.exe"));
  CHECK(!IsTrustedSetupUrl(
      L"https://github.com/teamdoticca/Phostide/releases/download/%2e%2e/"
      L"Phostide-Setup-1.0.0-win-x64.exe"));
  CHECK(!IsTrustedSetupUrl(L""));
}

TEST(SetupFileNameFromUrl_SafeNamesOnly) {
  CHECK(SetupFileNameFromUrl(L"https://x/y/Phostide-Setup-0.1.37-win-x64.exe") ==
        L"Phostide-Setup-0.1.37-win-x64.exe");
  CHECK(SetupFileNameFromUrl(L"https://x/y/Phostide-Setup-0.1.37-win-x64.exe?raw=1") ==
        L"Phostide-Setup-0.1.37-win-x64.exe");
  CHECK(SetupFileNameFromUrl(L"https://x/y/setup.zip").empty());
  CHECK(SetupFileNameFromUrl(L"https://x/y/a%20b.exe").empty());
  CHECK(SetupFileNameFromUrl(L"https://x/y/..exe").empty());
  CHECK(SetupFileNameFromUrl(L"no-slash.exe").empty());
}

TEST(IsAcceptedSetupSigner_Rules) {
  const std::wstring doticca =
      L"C=RO, S=Cluj, L=Cluj-Napoca, O=Doticca SRL, CN=Doticca SRL";
  // Signed host: exact same subject only.
  CHECK(IsAcceptedSetupSigner(doticca, doticca));
  CHECK(!IsAcceptedSetupSigner(L"CN=Someone Else", doticca));
  CHECK(!IsAcceptedSetupSigner(L"", doticca));
  // Unsigned dev host: Doticca publisher required.
  CHECK(IsAcceptedSetupSigner(doticca, L""));
  CHECK(!IsAcceptedSetupSigner(L"CN=Someone Else", L""));
  CHECK(!IsAcceptedSetupSigner(L"", L""));
}
