#include "UpdateCheck.h"

#include "../Common/PhostideCommon.h"

#include <windows.h>
#include <winhttp.h>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <string>
#include <string_view>

#pragma comment(lib, "winhttp.lib")

namespace phostide {
namespace {

constexpr wchar_t kApiHost[] = L"api.github.com";
constexpr wchar_t kApiPath[] =
    L"/repos/teamdoticca/Phostide/releases/latest";
constexpr wchar_t kFallbackReleasePage[] =
    L"https://github.com/teamdoticca/Phostide/releases/latest";

std::wstring Utf8ToWide(std::string_view utf8) {
  if (utf8.empty()) return {};
  const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(),
                                    static_cast<int>(utf8.size()), nullptr, 0);
  if (n <= 0) return {};
  std::wstring out(static_cast<size_t>(n), L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()),
                      out.data(), n);
  return out;
}

bool ParseTriple(std::wstring_view v, int& major, int& minor, int& patch) {
  major = minor = patch = 0;
  if (!v.empty() && (v.front() == L'v' || v.front() == L'V')) {
    v.remove_prefix(1);
  }
  if (v.empty()) return false;

  auto readPart = [&](int& out) -> bool {
    if (v.empty() || !iswdigit(v.front())) return false;
    out = 0;
    while (!v.empty() && iswdigit(v.front())) {
      out = out * 10 + (v.front() - L'0');
      v.remove_prefix(1);
    }
    return true;
  };

  if (!readPart(major)) return false;
  if (!v.empty() && v.front() == L'.') {
    v.remove_prefix(1);
    if (!readPart(minor)) return false;
  }
  if (!v.empty() && v.front() == L'.') {
    v.remove_prefix(1);
    if (!readPart(patch)) return false;
  }
  return true;
}

std::string ExtractJsonString(const std::string& json, const char* key) {
  const std::string needle = std::string("\"") + key + "\"";
  size_t pos = 0;
  while (true) {
    pos = json.find(needle, pos);
    if (pos == std::string::npos) return {};
    const size_t colon = json.find(':', pos + needle.size());
    if (colon == std::string::npos) return {};
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t' || json[i] == '\r' ||
                               json[i] == '\n')) {
      ++i;
    }
    if (i >= json.size() || json[i] != '"') {
      pos += needle.size();
      continue;
    }
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
}

// Prefer Phostide-Setup*.exe asset URL; else first *.exe matching Setup; else empty.
std::string FindSetupDownloadUrl(const std::string& json) {
  const std::string marker = "\"browser_download_url\"";
  size_t pos = 0;
  std::string fallback;
  while ((pos = json.find(marker, pos)) != std::string::npos) {
    const size_t colon = json.find(':', pos + marker.size());
    if (colon == std::string::npos) break;
    size_t i = colon + 1;
    while (i < json.size() && (json[i] == ' ' || json[i] == '\t' || json[i] == '\r' ||
                               json[i] == '\n')) {
      ++i;
    }
    if (i >= json.size() || json[i] != '"') {
      pos += marker.size();
      continue;
    }
    ++i;
    std::string url;
    while (i < json.size()) {
      const char c = json[i++];
      if (c == '\\' && i < json.size()) {
        url.push_back(json[i++]);
      } else if (c == '"') {
        break;
      } else {
        url.push_back(c);
      }
    }
    pos = i;
    // Case-insensitive name checks via URL path.
    std::string lower = url;
    for (char& c : lower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    if (lower.find("phostide-setup") != std::string::npos &&
        lower.find(".exe") != std::string::npos) {
      return url;
    }
    if (fallback.empty() && lower.find("setup") != std::string::npos &&
        lower.find(".exe") != std::string::npos) {
      fallback = url;
    }
  }
  return fallback;
}

bool HttpGetHttps(const wchar_t* host, const wchar_t* path, std::string& body,
                  std::wstring& error) {
  body.clear();
  HINTERNET session = WinHttpOpen(L"Phostide", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
  if (!session) {
    error = L"Could not open HTTP session.";
    return false;
  }

  DWORD timeout = 8000;
  WinHttpSetTimeouts(session, timeout, timeout, timeout, timeout);

  HINTERNET connect = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
  if (!connect) {
    WinHttpCloseHandle(session);
    error = L"Could not connect to GitHub.";
    return false;
  }

  HINTERNET request =
      WinHttpOpenRequest(connect, L"GET", path, nullptr, WINHTTP_NO_REFERER,
                         WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
  if (!request) {
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    error = L"Could not create HTTP request.";
    return false;
  }

  const wchar_t* headers =
      L"Accept: application/vnd.github+json\r\n"
      L"X-GitHub-Api-Version: 2022-11-28\r\n";
  BOOL sent = WinHttpSendRequest(request, headers, static_cast<DWORD>(-1L),
                                 WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
  if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    error = L"GitHub request failed.";
    return false;
  }

  DWORD status = 0;
  DWORD statusSize = sizeof(status);
  WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                      WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                      WINHTTP_NO_HEADER_INDEX);
  if (status < 200 || status >= 300) {
    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
    error = L"GitHub returned an error.";
    return false;
  }

  for (;;) {
    DWORD avail = 0;
    if (!WinHttpQueryDataAvailable(request, &avail)) {
      error = L"Failed reading response.";
      WinHttpCloseHandle(request);
      WinHttpCloseHandle(connect);
      WinHttpCloseHandle(session);
      return false;
    }
    if (avail == 0) break;
    if (body.size() + avail > 2 * 1024 * 1024) {
      error = L"Response too large.";
      WinHttpCloseHandle(request);
      WinHttpCloseHandle(connect);
      WinHttpCloseHandle(session);
      return false;
    }
    const size_t offset = body.size();
    body.resize(offset + avail);
    DWORD read = 0;
    if (!WinHttpReadData(request, body.data() + offset, avail, &read)) {
      error = L"Failed reading response body.";
      WinHttpCloseHandle(request);
      WinHttpCloseHandle(connect);
      WinHttpCloseHandle(session);
      return false;
    }
    body.resize(offset + read);
  }

  WinHttpCloseHandle(request);
  WinHttpCloseHandle(connect);
  WinHttpCloseHandle(session);
  return true;
}

}  // namespace

bool IsVersionNewer(std::wstring_view newerCandidate, std::wstring_view current) {
  int aMaj = 0, aMin = 0, aPat = 0;
  int bMaj = 0, bMin = 0, bPat = 0;
  if (!ParseTriple(newerCandidate, aMaj, aMin, aPat)) return false;
  if (!ParseTriple(current, bMaj, bMin, bPat)) return false;
  if (aMaj != bMaj) return aMaj > bMaj;
  if (aMin != bMin) return aMin > bMin;
  return aPat > bPat;
}

int64_t UtcNowUnixSeconds() {
  using namespace std::chrono;
  return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

bool IsUpdateCheckDue(int64_t lastUpdateCheckUtc) {
  if (lastUpdateCheckUtc <= 0) return true;
  constexpr int64_t kDay = 24 * 60 * 60;
  return (UtcNowUnixSeconds() - lastUpdateCheckUtc) >= kDay;
}

UpdateCheckResult CheckForUpdates() {
  UpdateCheckResult r;
  r.releasePageUrl = kFallbackReleasePage;

  std::string body;
  if (!HttpGetHttps(kApiHost, kApiPath, body, r.error)) {
    return r;
  }

  const std::string tagUtf8 = ExtractJsonString(body, "tag_name");
  if (tagUtf8.empty()) {
    r.error = L"Could not parse release tag.";
    return r;
  }
  r.latestTag = Utf8ToWide(tagUtf8);

  const std::string htmlUtf8 = ExtractJsonString(body, "html_url");
  if (!htmlUtf8.empty()) {
    r.releasePageUrl = Utf8ToWide(htmlUtf8);
  }

  const std::string setupUtf8 = FindSetupDownloadUrl(body);
  if (!setupUtf8.empty()) {
    r.setupUrl = Utf8ToWide(setupUtf8);
  } else {
    r.setupUrl = r.releasePageUrl;
  }

  r.ok = true;
  r.updateAvailable = IsVersionNewer(r.latestTag, kAppVersion);
  if (!r.updateAvailable) {
    // Clear semantics for callers that mirror into settings.
    r.setupUrl.clear();
  }
  return r;
}

}  // namespace phostide
