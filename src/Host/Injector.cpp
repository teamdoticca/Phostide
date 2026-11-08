#include "Injector.h"

#include "../Common/PhostideCommon.h"

#include <filesystem>
#include <string>

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

using HookProc = LRESULT(CALLBACK*)(int, WPARAM, LPARAM);

void LogInject(const std::wstring& line) {
  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  const fs::path dir = fs::path(temp) / L"Phostide";
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

}  // namespace

std::wstring TapDllPath() {
  return ModuleDir() + L"\\ExplorerTAP.dll";
}

bool InjectExplorerTap(std::wstring* errorOut) {
  const auto src = TapDllPath();
  if (!fs::exists(src)) {
    if (errorOut) *errorOut = L"ExplorerTAP.dll not found next to Phostide.exe";
    LogInject(L"missing dll");
    return false;
  }

  wchar_t temp[MAX_PATH]{};
  GetTempPathW(MAX_PATH, temp);
  const fs::path destDir = fs::path(temp) / L"Phostide";
  std::error_code ec;
  fs::create_directories(destDir, ec);
  const fs::path dest = destDir / L"ExplorerTAP.dll";
  fs::copy_file(src, dest, fs::copy_options::overwrite_existing, ec);
  if (ec) {
    if (errorOut) *errorOut = L"Failed to copy ExplorerTAP.dll to temp";
    LogInject(L"copy failed");
    return false;
  }
  LogInject(L"copied dll");

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

bool SendThemeCommand(const ThemeCommand& cmd) {
  HANDLE pipe = CreateFileW(L"\\\\.\\pipe\\Phostide.TAP", GENERIC_WRITE, 0, nullptr,
                            OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) return false;
  DWORD written = 0;
  const BOOL ok = WriteFile(pipe, &cmd, sizeof(cmd), &written, nullptr);
  CloseHandle(pipe);
  return ok && written == sizeof(cmd);
}

}  // namespace phostide
