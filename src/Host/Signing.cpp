#include "Signing.h"

#include <windows.h>
#include <softpub.h>
#include <wintrust.h>

#pragma comment(lib, "wintrust.lib")
#pragma comment(lib, "crypt32.lib")

namespace phostide {

std::wstring AuthenticodeSignerSubject(const std::wstring& file) {
  WINTRUST_FILE_INFO fileInfo{};
  fileInfo.cbStruct = sizeof(fileInfo);
  fileInfo.pcwszFilePath = file.c_str();

  WINTRUST_DATA data{};
  data.cbStruct = sizeof(data);
  data.dwUIChoice = WTD_UI_NONE;
  data.fdwRevocationChecks = WTD_REVOKE_NONE;
  data.dwUnionChoice = WTD_CHOICE_FILE;
  data.pFile = &fileInfo;
  data.dwStateAction = WTD_STATEACTION_VERIFY;
  data.dwProvFlags = WTD_CACHE_ONLY_URL_RETRIEVAL;

  GUID action = WINTRUST_ACTION_GENERIC_VERIFY_V2;
  const HWND noUi = static_cast<HWND>(INVALID_HANDLE_VALUE);
  std::wstring subject;
  if (WinVerifyTrust(noUi, &action, &data) == ERROR_SUCCESS) {
    CRYPT_PROVIDER_DATA* prov = WTHelperProvDataFromStateData(data.hWVTStateData);
    CRYPT_PROVIDER_SGNR* signer =
        prov ? WTHelperGetProvSignerFromChain(prov, 0, FALSE, 0) : nullptr;
    if (signer && signer->csCertChain > 0 && signer->pasCertChain &&
        signer->pasCertChain[0].pCert) {
      CERT_NAME_BLOB* name = &signer->pasCertChain[0].pCert->pCertInfo->Subject;
      const DWORD n = CertNameToStrW(X509_ASN_ENCODING, name, CERT_X500_NAME_STR, nullptr, 0);
      if (n > 1) {
        subject.resize(n);
        CertNameToStrW(X509_ASN_ENCODING, name, CERT_X500_NAME_STR, subject.data(), n);
        subject.resize(n - 1);
      }
    }
  }
  data.dwStateAction = WTD_STATEACTION_CLOSE;
  WinVerifyTrust(noUi, &action, &data);
  return subject;
}

const std::wstring& HostSignerSubject() {
  static const std::wstring subject = [] {
    wchar_t self[MAX_PATH]{};
    GetModuleFileNameW(nullptr, self, MAX_PATH);
    return AuthenticodeSignerSubject(self);
  }();
  return subject;
}

}  // namespace phostide
