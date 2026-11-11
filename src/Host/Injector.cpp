#include "Injector.h"

#include "../Common/PhostideCommon.h"

#include <filesystem>
#include <fstream>
#include <string>

#include <shellapi.h>

namespace fs = std::filesystem;

namespace phostide {
namespace {

HHOOK g_hook = nullptr;
HMODULE g_tapModule = nullptr;

std::wstring ModuleDir() {
  wchar_t path[MAX_PATH]{};
  GetModuleFileNameW(nullptr, path, MAX_PATH);
  return fs::path(path).parent_path().wstring();
}

fs::path TapTempDir() {
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  return fs::path(temp) / L"Phostide";
}

using HookProc = LRESULT(CALLBACK*)(int, WPARAM, LPARAM);

void LogInject(const std::wstring& line) {
  const fs::path dir = TapTempDir();
  std::error_code ec;
  fs::create_directories(dir, ec);
  const fs::path path = dir / L"inject.log";
  HANDLE f = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr, OPEN_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL, nullptr);
  if (f == INVALID_HANDLE_VALUE) return;
  SYSTEMTIME st{};
  GetLocalTime(&st);
  wchar_t prefix[64];
  swprintf_s(prefix, L"%02u:%02u:%02u ", st.wHour, st.wMinute, st.wSecond);
  std::wstring out = prefix + line + L"\r\n";
  DWORD written = 0;
  WriteFile(f, out.c_str(), static_cast<DWORD>(out.size() * sizeof(wchar_t)), &written, nullptr);
  CloseHandle(f);
}

std::wstring ReadTapVersionStamp() {
  const fs::path path = TapTempDir() / kTapVersionStamp;
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::string utf8;
  std::getline(in, utf8);
  if (utf8.empty()) return {};
  return std::wstring(utf8.begin(), utf8.end());
}

bool BoundEventIsSet() {
  HANDLE bound = OpenEventW(SYNCHRONIZE, FALSE, kTapBoundEvent.data());
  if (!bound) return false;
  const bool set = WaitForSingleObject(bound, 0) == WAIT_OBJECT_0;
  CloseHandle(bound);
  return set;
}

void ResetBoundEvent() {
  HANDLE bound = OpenEventW(EVENT_MODIFY_STATE, FALSE, kTapBoundEvent.data());
  if (!bound) return;
  ResetEvent(bound);
  CloseHandle(bound);
}

bool WaitForTray(DWORD timeoutMs) {
  const DWORD start = GetTickCount();
  while (GetTickCount() - start < timeoutMs) {
    if (FindWindowW(L"Shell_TrayWnd", nullptr)) return true;
    Sleep(200);
  }
  return FindWindowW(L"Shell_TrayWnd", nullptr) != nullptr;
}

bool RestartExplorerShell() {
  LogInject(L"restarting explorer shell");
  RequestTapRestore();
  ResetBoundEvent();

  STARTUPINFOW si{sizeof(si)};
  PROCESS_INFORMATION pi{};
  wchar_t cmd[] = L"taskkill.exe /IM explorer.exe /F";
  if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                     &si, &pi)) {
    WaitForSingleObject(pi.hProcess, 15000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  Sleep(1500);

  const std::wstring explorer = [] {
    wchar_t win[MAX_PATH]{};
    GetWindowsDirectoryW(win, MAX_PATH);
    return std::wstring(win) + L"\\explorer.exe";
  }();
  ShellExecuteW(nullptr, L"open", explorer.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

  if (!WaitForTray(30000)) {
    LogInject(L"explorer restart: tray not found");
    return false;
  }
  // Let the XAML taskbar island settle before ixde (same ritual as docs/development.md).
  Sleep(8000);
  LogInject(L"explorer restart: tray ready");
  return true;
}

bool NeedsExplorerBounceForFreshTap() {
  if (!BoundEventIsSet()) return false;
  const std::wstring stamped = ReadTapVersionStamp();
  if (stamped.empty()) {
    // Bound but no stamp (pre-0.1.10 TAP) — treat as stale.
    return true;
  }
  return stamped != kAppVersion;
}

bool CopyTapToTemp(fs::path& destOut, std::wstring* errorOut) {
  const auto src = TapDllPath();
  if (!fs::exists(src)) {
    if (errorOut) *errorOut = L"ExplorerTAP.dll not found next to Phostide.exe";
    LogInject(L"missing dll");
    return false;
  }

  const fs::path destDir = TapTempDir();
  std::error_code ec;
  fs::create_directories(destDir, ec);

  // Always version the TEMP filename so an upgrade never needs to overwrite a
  // locked ExplorerTAP.dll from a previous build still mapped in Explorer.
  destOut = destDir / (std::wstring(L"ExplorerTAP-") + std::wstring(kAppVersion) + L".dll");
  fs::copy_file(src, destOut, fs::copy_options::overwrite_existing, ec);
  if (!ec) {
    LogInject(L"copied versioned dll");
    return true;
  }

  std::error_code se;
  const auto srcSize = fs::file_size(src, se);
  const auto dstSize = fs::exists(destOut) ? fs::file_size(destOut, se) : static_cast<uintmax_t>(-1);
  if (fs::exists(destOut) && srcSize == dstSize) {
    LogInject(L"copy locked; reusing versioned temp dll (same size)");
    return true;
  }

  if (errorOut) *errorOut = L"Failed to copy ExplorerTAP.dll to temp";
  LogInject(L"copy failed");
  return false;
}

bool HookTapIntoTray(const fs::path& dest, std::wstring* errorOut) {
  HWND tray = nullptr;
  for (int i = 0; i < 40 && !tray; ++i) {
    tray = FindWindowW(L"Shell_TrayWnd", nullptr);
    if (!tray) Sleep(150);
  }
  if (!tray) {
    if (errorOut) *errorOut = L"Shell_TrayWnd not found";
    LogInject(L"no tray");
    return false;
  }
  DWORD pid = 0;
  const DWORD tid = GetWindowThreadProcessId(tray, &pid);
  if (!tid) {
    if (errorOut) *errorOut = L"Could not get taskbar thread id";
    LogInject(L"no tid");
    return false;
  }
  LogInject(L"tid=" + std::to_wstring(tid) + L" pid=" + std::to_wstring(pid));

  // Drop any previous host-side hook/module before loading a new build.
  DetachExplorerTap();

  g_tapModule = LoadLibraryW(dest.c_str());
  if (!g_tapModule) {
    if (errorOut) *errorOut = L"LoadLibrary(ExplorerTAP) failed in host";
    LogInject(L"LoadLibrary failed " + std::to_wstring(GetLastError()));
    return false;
  }

  auto proc = reinterpret_cast<HookProc>(GetProcAddress(g_tapModule, "PhostideCallWndProc"));
  if (!proc) {
    if (errorOut) *errorOut = L"PhostideCallWndProc export missing";
    LogInject(L"missing export");
    return false;
  }

  g_hook = SetWindowsHookExW(WH_CALLWNDPROC, proc, g_tapModule, tid);
  if (!g_hook) {
    if (errorOut) *errorOut = L"SetWindowsHookEx failed";
    LogInject(L"hook failed " + std::to_wstring(GetLastError()));
    return false;
  }
  LogInject(L"hook ok");

  for (int i = 0; i < 10; ++i) {
    PostMessageW(tray, WM_NULL, 0, 0);
    SendMessageTimeoutW(tray, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 500, nullptr);
    Sleep(150);
  }

  const HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, kTapReadyEvent.data());
  const DWORD wait = ready ? WaitForSingleObject(ready, 25000) : WAIT_FAILED;
  if (ready) CloseHandle(ready);
  LogInject(L"ready wait=" + std::to_wstring(wait));
  return true;
}

}  // namespace

std::wstring TapDllPath() {
  return ModuleDir() + L"\\ExplorerTAP.dll";
}

bool InjectExplorerTap(std::wstring* errorOut) {
  // Stale in-Explorer TAP (previous version) cannot be unloaded without a shell
  // bounce — XAML Diagnostics pins the module for Explorer's lifetime.
  if (NeedsExplorerBounceForFreshTap()) {
    LogInject(L"stale in-explorer TAP detected");
    if (!RestartExplorerShell()) {
      if (errorOut) *errorOut = L"Failed to restart Explorer for TAP upgrade";
      return false;
    }
  }

  fs::path dest;
  if (!CopyTapToTemp(dest, errorOut)) {
    // Last resort: bounce shell to unlock any leftover mapping, then retry once.
    LogInject(L"copy failed; retry after explorer restart");
    if (!RestartExplorerShell() || !CopyTapToTemp(dest, errorOut)) {
      return false;
    }
  }

  return HookTapIntoTray(dest, errorOut);
}

bool SendThemeCommand(const ThemeCommand& cmd) {
  HANDLE pipe = CreateFileW(L"\\\\.\\pipe\\Phostide.TAP", GENERIC_WRITE, 0, nullptr,
                            OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) return false;
  DWORD written = 0;
  const BOOL ok = WriteFile(pipe, &cmd, sizeof(cmd), &written, nullptr);
  CloseHandle(pipe);
  return ok && written == sizeof(cmd);
}

bool RequestTapRestore() {
  ThemeCommand cmd{};
  cmd.apiVersion = kTapApiVersion;
  cmd.enabled = 0;
  return SendThemeCommand(cmd);
}

void DetachExplorerTap() {
  if (g_hook) {
    UnhookWindowsHookEx(g_hook);
    g_hook = nullptr;
    LogInject(L"unhooked");
  }
  if (g_tapModule) {
    FreeLibrary(g_tapModule);
    g_tapModule = nullptr;
    LogInject(L"freed tap module");
  }
}

bool RequestExistingInstanceExit(DWORD timeoutMs) {
  HWND hwnd = FindWindowW(L"PhostideTray", nullptr);
  if (!hwnd) {
    // No window — still wait briefly in case mutex is held during teardown.
    HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, kMutexName.data());
    if (!existing) return true;
    const DWORD wait = WaitForSingleObject(existing, timeoutMs);
    CloseHandle(existing);
    return wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED;
  }

  PostMessageW(hwnd, WM_CLOSE, 0, 0);
  // Nudge the UI thread in case it is sitting in a modal / SendMessage.
  SendNotifyMessageW(hwnd, WM_NULL, 0, 0);

  HANDLE existing = OpenMutexW(SYNCHRONIZE, FALSE, kMutexName.data());
  if (!existing) return true;
  const DWORD wait = WaitForSingleObject(existing, timeoutMs);
  CloseHandle(existing);
  LogInject(wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED
                ? L"exit signal: mutex released"
                : L"exit signal: mutex wait timed out");
  return wait == WAIT_OBJECT_0 || wait == WAIT_ABANDONED;
}

}  // namespace phostide
