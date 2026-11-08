#include "TapSite.h"
#include "TaskbarAppearanceService.h"
#include "../Common/PhostideCommon.h"

#include <atomic>
#include <cstring>

#include <windows.h>
#include <winrt/base.h>

namespace {
std::atomic<bool> g_initialized{false};
HMODULE g_module = nullptr;
constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\Phostide.TAP";

bool IsExplorerProcess() {
  wchar_t path[MAX_PATH]{};
  if (!GetModuleFileNameW(nullptr, path, MAX_PATH)) return false;
  const wchar_t* base = wcsrchr(path, L'\\');
  base = base ? base + 1 : path;
  return _wcsicmp(base, L"explorer.exe") == 0;
}

DWORD WINAPI PipeThread(LPVOID) {
  while (true) {
    HANDLE pipe = CreateNamedPipeW(
        kPipeName, PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1, sizeof(phostide::ThemeCommand), sizeof(phostide::ThemeCommand), 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
      Sleep(1000);
      continue;
    }
    const BOOL connected = ConnectNamedPipe(pipe, nullptr)
                               ? TRUE
                               : (GetLastError() == ERROR_PIPE_CONNECTED);
    if (connected) {
      phostide::ThemeCommand cmd{};
      DWORD read = 0;
      if (ReadFile(pipe, &cmd, sizeof(cmd), &read, nullptr) && read == sizeof(cmd)) {
        if (cmd.apiVersion == phostide::kTapApiVersion) {
          phostide::TaskbarAppearanceService::Instance().Apply(cmd);
        }
      }
    }
    DisconnectNamedPipe(pipe);
    CloseHandle(pipe);
  }
  return 0;
}

DWORD WINAPI InitThread(LPVOID) {
  // Pipe + host-death watch first (safe, no XAML).
  CreateThread(nullptr, 0, PipeThread, nullptr, 0, nullptr);
  phostide::TaskbarAppearanceService::Instance().WatchHostMutex();

  // XAML Diagnostics bootstrap (may take retries). Signals kTapReadyEvent when advised.
  phostide::TAPSite::Install(nullptr);
  return 0;
}

struct ClassFactory : winrt::implements<ClassFactory, IClassFactory> {
  HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID iid, void** result) noexcept override {
    *result = nullptr;
    if (outer) return CLASS_E_NOAGGREGATION;
    return winrt::make<phostide::TAPSite>()->QueryInterface(iid, result);
  }
  HRESULT STDMETHODCALLTYPE LockServer(BOOL) noexcept override { return S_OK; }
};

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_module = module;
    DisableThreadLibraryCalls(module);
    // Host also LoadLibrary's this DLL for SetWindowsHookEx — never start TAP there.
    if (IsExplorerProcess() && !g_initialized.exchange(true)) {
      CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
  }
  return TRUE;
}

extern "C" HRESULT WINAPI DllGetClassObject(REFCLSID clsid, REFIID iid, void** result) {
  *result = nullptr;
  if (InlineIsEqualGUID(clsid, phostide::kTapSiteClsid)) {
    return winrt::make<ClassFactory>()->QueryInterface(iid, result);
  }
  return CLASS_E_CLASSNOTAVAILABLE;
}

extern "C" HRESULT WINAPI DllCanUnloadNow() {
  // InitializeXamlDiagnosticsEx pins this module for Explorer's lifetime.
  return S_FALSE;
}

extern "C" __declspec(dllexport) void PhostideTapNotifyHostPid(DWORD) {}

extern "C" __declspec(dllexport) void PhostideTapApply(const phostide::ThemeCommand* cmd) {
  if (cmd && IsExplorerProcess()) {
    phostide::TaskbarAppearanceService::Instance().Apply(*cmd);
  }
}

extern "C" __declspec(dllexport) int PhostideTapApiVersion() {
  return phostide::kTapApiVersion;
}

extern "C" __declspec(dllexport) LRESULT CALLBACK PhostideCallWndProc(int code, WPARAM wParam,
                                                                      LPARAM lParam) {
  return CallNextHookEx(nullptr, code, wParam, lParam);
}
