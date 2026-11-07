#include "TaskbarAppearanceService.h"

#include <windows.h>

#include <atomic>

namespace phostide {
bool StartXamlDiagnosticsBootstrap();
void ScanFromRoot(winrt::Windows::UI::Xaml::DependencyObject const& root);
}

namespace {
std::atomic<bool> g_initialized{false};
HMODULE g_module = nullptr;

// Named pipe for host → TAP theme commands.
constexpr wchar_t kPipeName[] = L"\\\\.\\pipe\\Phostide.TAP";

DWORD WINAPI PipeThread(LPVOID) {
  while (true) {
    HANDLE pipe = CreateNamedPipeW(
        kPipeName, PIPE_ACCESS_INBOUND,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        PIPE_UNLIMITED_INSTANCES, sizeof(phostide::ThemeCommand),
        sizeof(phostide::ThemeCommand), 0, nullptr);
    if (pipe == INVALID_HANDLE_VALUE) {
      Sleep(500);
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
  winrt::init_apartment(winrt::apartment_type::multi_threaded);
  phostide::StartXamlDiagnosticsBootstrap();

  if (HANDLE ready = CreateEventW(nullptr, TRUE, FALSE, phostide::kTapReadyEvent.data())) {
    SetEvent(ready);
    CloseHandle(ready);
  }

  CreateThread(nullptr, 0, PipeThread, nullptr, 0, nullptr);
  return 0;
}
}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
  if (reason == DLL_PROCESS_ATTACH) {
    g_module = module;
    DisableThreadLibraryCalls(module);
    if (!g_initialized.exchange(true)) {
      CreateThread(nullptr, 0, InitThread, nullptr, 0, nullptr);
    }
  } else if (reason == DLL_PROCESS_DETACH) {
    phostide::TaskbarAppearanceService::Instance().StopRenderLoop();
  }
  return TRUE;
}

// Exported helpers for the host injector / diagnostics site.
extern "C" __declspec(dllexport) void PhostideTapNotifyHostPid(DWORD pid) {
  phostide::TaskbarAppearanceService::Instance().WatchHostProcess(pid);
}

extern "C" __declspec(dllexport) void PhostideTapApply(const phostide::ThemeCommand* cmd) {
  if (cmd) {
    phostide::TaskbarAppearanceService::Instance().Apply(*cmd);
  }
}

extern "C" __declspec(dllexport) int PhostideTapApiVersion() {
  return phostide::kTapApiVersion;
}
