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
  fs::path p(path);
  return p.parent_path().wstring();
}

LRESULT CALLBACK CallWndProc(int code, WPARAM wParam, LPARAM lParam) {
  // Loading the DLL into the remote process happens as a side effect of the hook.
  return CallNextHookEx(g_hook, code, wParam, lParam);
}

}  // namespace

std::wstring TapDllPath() {
  return ModuleDir() + L"\\ExplorerTAP.dll";
}

bool InjectExplorerTap(std::wstring* errorOut) {
  const auto src = TapDllPath();
  if (!fs::exists(src)) {
    if (errorOut) *errorOut = L"ExplorerTAP.dll not found next to Phostide.exe";
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
    return false;
  }

  HWND tray = FindWindowW(L"Shell_TrayWnd", nullptr);
  if (!tray) {
    if (errorOut) *errorOut = L"Shell_TrayWnd not found";
    return false;
  }
  DWORD tid = GetWindowThreadProcessId(tray, nullptr);
  if (!tid) {
    if (errorOut) *errorOut = L"Could not get taskbar thread id";
    return false;
  }

  // Load the DLL in *this* process first so the hook procedure module is valid,
  // then install a thread-specific hook so Windows maps it into Explorer.
  g_tapModule = LoadLibraryW(dest.c_str());
  if (!g_tapModule) {
    if (errorOut) *errorOut = L"LoadLibrary(ExplorerTAP) failed in host";
    return false;
  }

  g_hook = SetWindowsHookExW(WH_CALLWNDPROC, CallWndProc, g_tapModule, tid);
  if (!g_hook) {
    if (errorOut) *errorOut = L"SetWindowsHookEx failed";
    return false;
  }

  // Nudge the taskbar thread so the hook runs and Explorer loads the module.
  PostMessageW(tray, WM_NULL, 0, 0);
  SendMessageTimeoutW(tray, WM_NULL, 0, 0, SMTO_ABORTIFHUNG, 1000, nullptr);

  // Signal host pid to TAP if export exists.
  using NotifyFn = void(__stdcall*)(DWORD);
  if (auto notify = reinterpret_cast<NotifyFn>(
          GetProcAddress(g_tapModule, "PhostideTapNotifyHostPid"))) {
    notify(GetCurrentProcessId());
  }

  HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, kTapReadyEvent.data());
  if (ready) {
    WaitForSingleObject(ready, 5000);
    CloseHandle(ready);
  }
  return true;
}

bool SendThemeCommand(const ThemeCommand& cmd) {
  if (!WaitNamedPipeW(L"\\\\.\\pipe\\Phostide.TAP", 2000)) {
    // Pipe may not be up yet — still try.
  }
  HANDLE pipe = CreateFileW(L"\\\\.\\pipe\\Phostide.TAP", GENERIC_WRITE, 0, nullptr,
                            OPEN_EXISTING, 0, nullptr);
  if (pipe == INVALID_HANDLE_VALUE) return false;
  DWORD written = 0;
  const BOOL ok = WriteFile(pipe, &cmd, sizeof(cmd), &written, nullptr);
  CloseHandle(pipe);
  return ok && written == sizeof(cmd);
}

}  // namespace phostide
