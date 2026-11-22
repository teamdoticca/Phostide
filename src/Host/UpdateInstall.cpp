#include "UpdateInstall.h"

#include "Signing.h"
#include "UpdateCheck.h"
#include "../Common/PhostideCommon.h"

#include <windows.h>
#include <shellapi.h>
#include <winhttp.h>

#include <algorithm>
#include <cwctype>
#include <filesystem>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "version.lib")

namespace phostide {
namespace {

namespace fs = std::filesystem;

constexpr std::wstring_view kReleaseDownloadPrefix =
    L"https://github.com/teamdoticca/Phostide/releases/download/";
constexpr uint64_t kMaxSetupBytes = 64ull * 1024 * 1024;
constexpr std::wstring_view kDoticcaPublisher = L"CN=Doticca SRL";

fs::path UpdateDir() {
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  return fs::path(temp) / L"Phostide" / L"update";
}

std::wstring LowerAscii(std::wstring_view s) {
  std::wstring out(s);
  std::transform(out.begin(), out.end(), out.begin(),
                 [](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
  return out;
}

// "0.1.37" from the Setup's VS_FIXEDFILEINFO (Inno: VersionInfoVersion=x.y.z.0).
std::wstring FileVersionTriple(const std::wstring& path) {
  DWORD handle = 0;
  const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
  if (size == 0) return {};
  std::vector<BYTE> buf(size);
  if (!GetFileVersionInfoW(path.c_str(), 0, size, buf.data())) return {};
  VS_FIXEDFILEINFO* info = nullptr;
  UINT len = 0;
  if (!VerQueryValueW(buf.data(), L"\\", reinterpret_cast<void**>(&info), &len) || !info) {
    return {};
  }
  wchar_t out[64];
  swprintf_s(out, L"%u.%u.%u", HIWORD(info->dwFileVersionMS), LOWORD(info->dwFileVersionMS),
             HIWORD(info->dwFileVersionLS));
  return out;
}

struct WinHttpHandles {
  HINTERNET session = nullptr;
  HINTERNET connect = nullptr;
  HINTERNET request = nullptr;
  ~WinHttpHandles() {
    if (request) WinHttpCloseHandle(request);
    if (connect) WinHttpCloseHandle(connect);
    if (session) WinHttpCloseHandle(session);
  }
};

bool DownloadToFile(const std::wstring& url, const fs::path& dest, DownloadProgressFn progress,
                    void* context, std::wstring& error) {
  URL_COMPONENTS parts{sizeof(parts)};
  wchar_t host[256]{};
  wchar_t path[2048]{};
  parts.lpszHostName = host;
  parts.dwHostNameLength = ARRAYSIZE(host);
  parts.lpszUrlPath = path;
  parts.dwUrlPathLength = ARRAYSIZE(path);
  if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS) {
    error = L"Invalid download link.";
    return false;
  }

  WinHttpHandles h;
  h.session = WinHttpOpen(L"Phostide", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS, 0);
  if (!h.session) {
    error = L"Could not open an HTTP session.";
    return false;
  }
  WinHttpSetTimeouts(h.session, 15000, 15000, 15000, 30000);
  // GitHub redirects release assets to its CDN; never allow HTTPS -> HTTP.
  DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
  WinHttpSetOption(h.session, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));

  h.connect = WinHttpConnect(h.session, host, parts.nPort, 0);
  if (!h.connect) {
    error = L"Could not connect to GitHub.";
    return false;
  }
  h.request = WinHttpOpenRequest(h.connect, L"GET", path, nullptr, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!h.request ||
      !WinHttpSendRequest(h.request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA,
                          0, 0, 0) ||
      !WinHttpReceiveResponse(h.request, nullptr)) {
    error = L"Download failed (network).";
    return false;
  }

  DWORD status = 0;
  DWORD statusSize = sizeof(status);
  WinHttpQueryHeaders(h.request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                      WINHTTP_NO_HEADER_INDEX);
  if (status != 200) {
    error = L"Download failed (HTTP " + std::to_wstring(status) + L").";
    return false;
  }
  DWORD total = 0;
  DWORD totalSize = sizeof(total);
  if (!WinHttpQueryHeaders(h.request, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                           WINHTTP_HEADER_NAME_BY_INDEX, &total, &totalSize,
                           WINHTTP_NO_HEADER_INDEX)) {
    total = 0;
  }
  if (total > kMaxSetupBytes) {
    error = L"Download is unexpectedly large.";
    return false;
  }

  HANDLE file = CreateFileW(dest.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (file == INVALID_HANDLE_VALUE) {
    error = L"Could not write the download to %TEMP%.";
    return false;
  }

  uint64_t received = 0;
  int lastPercent = -2;
  std::vector<char> chunk(64 * 1024);
  bool ok = true;
  for (;;) {
    DWORD read = 0;
    if (!WinHttpReadData(h.request, chunk.data(), static_cast<DWORD>(chunk.size()), &read)) {
      error = L"Download interrupted.";
      ok = false;
      break;
    }
    if (read == 0) break;
    received += read;
    if (received > kMaxSetupBytes) {
      error = L"Download is unexpectedly large.";
      ok = false;
      break;
    }
    DWORD written = 0;
    if (!WriteFile(file, chunk.data(), read, &written, nullptr) || written != read) {
      error = L"Could not write the download to %TEMP%.";
      ok = false;
      break;
    }
    const int percent = total ? static_cast<int>(received * 100 / total) : -1;
    if (progress && percent != lastPercent) {
      lastPercent = percent;
      progress(context, percent);
    }
  }
  CloseHandle(file);
  if (ok && total && received != total) {
    error = L"Download incomplete.";
    ok = false;
  }
  if (!ok) {
    std::error_code ec;
    fs::remove(dest, ec);
  }
  return ok;
}

}  // namespace

bool IsTrustedSetupUrl(std::wstring_view url) {
  if (url.size() <= kReleaseDownloadPrefix.size()) return false;
  // No path tricks that could leave this repo's releases (signature is still checked).
  if (url.find(L"..") != std::wstring_view::npos || url.find(L'%') != std::wstring_view::npos ||
      url.find(L'\\') != std::wstring_view::npos) {
    return false;
  }
  if (LowerAscii(url.substr(0, kReleaseDownloadPrefix.size())) !=
      LowerAscii(kReleaseDownloadPrefix)) {
    return false;
  }
  const std::wstring name = SetupFileNameFromUrl(url);
  return !name.empty() && LowerAscii(name).rfind(L"phostide-setup-", 0) == 0;
}

std::wstring SetupFileNameFromUrl(std::wstring_view url) {
  const size_t query = url.find_first_of(L"?#");
  if (query != std::wstring_view::npos) url = url.substr(0, query);
  const size_t slash = url.rfind(L'/');
  if (slash == std::wstring_view::npos) return {};
  std::wstring name(url.substr(slash + 1));
  if (name.size() < 5 || name.size() > 100) return {};
  for (wchar_t c : name) {
    const bool safe = (c >= L'a' && c <= L'z') || (c >= L'A' && c <= L'Z') ||
                      (c >= L'0' && c <= L'9') || c == L'-' || c == L'_' || c == L'.';
    if (!safe) return {};
  }
  if (LowerAscii(name.substr(name.size() - 4)) != L".exe") return {};
  if (name.find(L"..") != std::wstring::npos) return {};
  return name;
}

bool IsAcceptedSetupSigner(std::wstring_view setupSigner, std::wstring_view hostSigner) {
  if (setupSigner.empty()) return false;
  if (!hostSigner.empty()) return setupSigner == hostSigner;
  return setupSigner.find(kDoticcaPublisher) != std::wstring_view::npos;
}

SetupDownloadResult DownloadAndVerifySetup(const std::wstring& url, DownloadProgressFn progress,
                                           void* context) {
  SetupDownloadResult r;
  if (!IsTrustedSetupUrl(url)) {
    r.error = L"Not a Phostide release download.";
    return r;
  }
  const fs::path dir = UpdateDir();
  std::error_code ec;
  fs::create_directories(dir, ec);
  // Drop earlier downloads; a Setup still running is locked and simply stays.
  for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec)) {
    std::error_code rm;
    fs::remove(it->path(), rm);
  }

  const fs::path dest = dir / SetupFileNameFromUrl(url);
  if (!DownloadToFile(url, dest, progress, context, r.error)) return r;

  const std::wstring signer = AuthenticodeSignerSubject(dest.wstring());
  if (!IsAcceptedSetupSigner(signer, HostSignerSubject())) {
    fs::remove(dest, ec);
    r.error = L"The download failed signature verification.";
    return r;
  }
  const std::wstring version = FileVersionTriple(dest.wstring());
  if (!IsVersionNewer(version, kAppVersion)) {
    fs::remove(dest, ec);
    r.error = L"The download is not newer than this version.";
    return r;
  }
  r.ok = true;
  r.setupPath = dest.wstring();
  return r;
}

bool LaunchSetupForUpdate(const std::wstring& setupPath, std::wstring* error) {
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  const std::wstring log = (fs::path(temp) / L"Phostide" / L"setup.log").wstring();
  // /RELAUNCH=1: Setup starts Phostide again when it is done (installer [Code]).
  const std::wstring params =
      L"/SILENT /SUPPRESSMSGBOXES /NORESTART /RELAUNCH=1 /LOG=\"" + log + L"\"";
  SHELLEXECUTEINFOW sei{sizeof(sei)};
  sei.fMask = SEE_MASK_NOASYNC;
  sei.lpVerb = L"open";
  sei.lpFile = setupPath.c_str();
  sei.lpParameters = params.c_str();
  sei.nShow = SW_SHOWNORMAL;
  if (!ShellExecuteExW(&sei)) {
    if (error) *error = L"Could not start the installer.";
    return false;
  }
  return true;
}

}  // namespace phostide
