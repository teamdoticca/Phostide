#include "Injector.h"

#include "../Common/PhostideCommon.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>

#include <shellapi.h>

namespace fs = std::filesystem;

namespace phostide {
namespace {

HHOOK g_hook = nullptr;
HMODULE g_tapModule = nullptr;

// Shell bounce state, so a host Exit in the middle of RestartExplorerShell never
// leaves the user without Explorer (taskkill done, relaunch not yet issued).
enum ShellPhase : int { kShellIdle = 0, kShellKilling = 1, kShellNeedsLaunch = 2 };
std::atomic<int> g_shellPhase{kShellIdle};

std::wstring ExplorerPath() {
  wchar_t win[MAX_PATH]{};
  GetWindowsDirectoryW(win, MAX_PATH);
  return std::wstring(win) + L"\\explorer.exe";
}

// Exactly one caller (restart worker or shutdown) relaunches Explorer.
bool LaunchExplorerIfPending() {
  int expected = kShellNeedsLaunch;
  if (!g_shellPhase.compare_exchange_strong(expected, kShellIdle)) return false;
  ShellExecuteW(nullptr, L"open", ExplorerPath().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  return true;
}

// ExplorerTAP keeps its Ready event handle open for Explorer's lifetime
// (TapSite::Install), so the named event exists only while a TAP is mapped into
// the running shell. After a shell restart or reboot it is gone.
bool TapLoadedInExplorer() {
  HANDLE ev = OpenEventW(SYNCHRONIZE, FALSE, kTapReadyEvent.data());
  if (!ev) return false;
  CloseHandle(ev);
  return true;
}

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

void ExplainExplorerRestartToUser() {
  // Inno Setup's wizard can cover a MessageBox and look "stuck". If Setup is
  // still open, it already shows status text — skip the dialog.
  if (FindWindowW(L"TWizardForm", nullptr) ||
      FindWindowW(L"TUninstallProgressForm", nullptr)) {
    LogInject(L"skip restart dialog; Setup UI is active");
    return;
  }
  MessageBoxW(
      nullptr,
      L"Phostide needs to briefly restart Windows Explorer so the taskbar "
      L"theme engine can update.\n\n"
      L"Your desktop may flicker for a few seconds. This is expected - "
      L"open windows stay open.",
      L"Phostide", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND | MB_TOPMOST);
}

bool RestartExplorerShell(bool explainToUser) {
  if (explainToUser) {
    ExplainExplorerRestartToUser();
  }
  LogInject(L"restarting explorer shell");
  RequestTapRestore();
  ResetBoundEvent();

  g_shellPhase.store(kShellKilling);
  STARTUPINFOW si{sizeof(si)};
  PROCESS_INFORMATION pi{};
  wchar_t cmd[] = L"taskkill.exe /IM explorer.exe /F";
  if (CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr,
                     &si, &pi)) {
    WaitForSingleObject(pi.hProcess, 15000);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
  }
  g_shellPhase.store(kShellNeedsLaunch);
  Sleep(1500);

  // Shutdown may already have relaunched it (FinishPendingExplorerRestart).
  LaunchExplorerIfPending();

  if (!WaitForTray(30000)) {
    LogInject(L"explorer restart: tray not found");
    return false;
  }
  // XAML Diagnostics (ixde) returns ERROR_NOT_FOUND if the island is still
  // warming — and once that sticks, the Explorer process may never advise.
  // 8s was too short after TaskbarCreated storms; wait longer.
  Sleep(15000);
  LogInject(L"explorer restart: tray ready");
  return true;
}

bool NeedsExplorerBounceForFreshTap() {
  // No TAP mapped into this Explorer (first run, after reboot, or Setup already
  // bounced the shell): the new DLL loads fresh — nothing stale to unload. The
  // stamp file outlives Explorer, so checking it alone caused a second bounce
  // right after Setup's own restart.
  if (!TapLoadedInExplorer()) return false;
  std::wstring stamped = ReadTapVersionStamp();
  // The TAP rewrites the stamp (CREATE_ALWAYS) on every bind, so a read can land
  // between truncate and write while Bound is already set — that looked like a
  // pre-0.1.10 TAP and bounced Explorer. Retry briefly if the file exists.
  const fs::path stampPath = TapTempDir() / kTapVersionStamp;
  for (int i = 0; stamped.empty() && i < 10 && fs::exists(stampPath); ++i) {
    Sleep(50);
    stamped = ReadTapVersionStamp();
  }
  // Bounce whenever the on-disk stamp disagrees with this host — even if Bound
  // is clear. Skipping that case left Explorer mapped to an old TAP that held
  // the SingleInstance mutex and blocked host restart after Kill-Process.
  if (stamped.empty()) {
    return BoundEventIsSet();  // pre-0.1.10 TAP still painting
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
  // GPU pixel shaders — required next to the loaded TAP module / TEMP fallback.
  {
    std::error_code hlslEc;
    const fs::path hlslSrc = fs::path(src).parent_path() / L"Themes.hlsl";
    const fs::path hlslDst = destDir / L"Themes.hlsl";
    if (fs::exists(hlslSrc)) {
      fs::copy_file(hlslSrc, hlslDst, fs::copy_options::overwrite_existing, hlslEc);
    }
  }
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

void ResetTapBoundEvent() { ResetBoundEvent(); }

bool InjectExplorerTap(std::wstring* errorOut, const ThemeCommand* reapply) {
  // No ResetBoundEvent here: callers reset before scheduling inject (host startup,
  // tray recreate) and RestartExplorerShell resets its own. Resetting again raced
  // the host's startup Apply — a TAP already loaded in this Explorer rebinds on that
  // Apply, the reset wiped its Bound signal, and Exit → relaunch bounced Explorer.

  // Stale in-Explorer TAP (previous version) cannot be unloaded without a shell
  // bounce — XAML Diagnostics pins the module for Explorer's lifetime.
  if (NeedsExplorerBounceForFreshTap()) {
    LogInject(L"stale in-explorer TAP detected");
    if (!RestartExplorerShell(/*explainToUser=*/true)) {
      if (errorOut) *errorOut = L"Failed to restart Explorer for TAP upgrade";
      return false;
    }
  }

  fs::path dest;
  if (!CopyTapToTemp(dest, errorOut)) {
    // Last resort: bounce shell to unlock any leftover mapping, then retry once.
    LogInject(L"copy failed; retry after explorer restart");
    if (!RestartExplorerShell(/*explainToUser=*/true) || !CopyTapToTemp(dest, errorOut)) {
      return false;
    }
  }

  if (!HookTapIntoTray(dest, errorOut)) {
    return false;
  }
  // Host relaunch into the same Explorer (Exit → start again): the TAP is still
  // loaded, so Ready is already set and nothing binds on its own — fills are
  // soft-suspended until a command arrives. The host's startup Apply may have
  // rebound them *before* ResetBoundEvent above cleared the signal, so re-send
  // the command and give the TAP dispatcher a moment before bouncing Explorer.
  if (!BoundEventIsSet() && reapply && reapply->enabled && !reapply->paused) {
    LogInject(L"not Bound after hook; reapply command and wait");
    SendThemeCommand(*reapply);
    for (int i = 0; i < 30 && !BoundEventIsSet(); ++i) Sleep(100);
    if (BoundEventIsSet()) LogInject(L"Bound after reapply (no explorer restart)");
  }
  // ready event can fire after ixde gave up (TapSite signals anyway). Require Bound.
  if (!BoundEventIsSet()) {
    LogInject(L"TAP ready but not Bound — retry after explorer restart");
    if (!RestartExplorerShell(/*explainToUser=*/false)) {
      if (errorOut) *errorOut = L"ExplorerTAP installed but BackgroundFill never bound";
      return false;
    }
    if (!CopyTapToTemp(dest, errorOut)) return false;
    if (!HookTapIntoTray(dest, errorOut)) return false;
    if (!BoundEventIsSet()) {
      if (errorOut) *errorOut = L"ExplorerTAP installed but BackgroundFill never bound";
      return false;
    }
  }
  return true;
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

void FinishPendingExplorerRestart() {
  // taskkill may still be running on the inject worker; let it finish (it waits
  // at most 15 s) so we do not relaunch Explorer just before it gets killed.
  for (int i = 0; i < 170 && g_shellPhase.load() == kShellKilling; ++i) Sleep(100);
  if (g_shellPhase.load() == kShellKilling) {
    // taskkill hung — assume it is done rather than risk no shell at all.
    int expected = kShellKilling;
    g_shellPhase.compare_exchange_strong(expected, kShellNeedsLaunch);
  }
  if (g_shellPhase.load() != kShellNeedsLaunch) return;
  Sleep(500);  // let the killed shell finish tearing down its tray
  if (LaunchExplorerIfPending()) {
    LogInject(L"shutdown: relaunched explorer interrupted mid-restart");
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
