#include "MonitorEnum.h"

#include <algorithm>

namespace phostide {
namespace {

struct EnumCtx {
  std::vector<AttachedMonitor>* out = nullptr;
};

BOOL CALLBACK EnumProc(HMONITOR monitor, HDC, LPRECT, LPARAM lParam) {
  auto* ctx = reinterpret_cast<EnumCtx*>(lParam);
  MONITORINFOEXW mi{};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(monitor, &mi)) return TRUE;

  AttachedMonitor m;
  m.deviceName = mi.szDevice;
  m.deviceHash = HashMonitorDeviceName(m.deviceName);
  m.rc = mi.rcMonitor;
  m.width = (std::max)(1, static_cast<int>(mi.rcMonitor.right - mi.rcMonitor.left));
  m.height = (std::max)(1, static_cast<int>(mi.rcMonitor.bottom - mi.rcMonitor.top));
  m.primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
  ctx->out->push_back(std::move(m));
  return TRUE;
}

}  // namespace

uint64_t HashMonitorDeviceName(std::wstring_view deviceName) {
  uint64_t hash = 14695981039346656037ull;
  for (const wchar_t ch : deviceName) {
    hash ^= static_cast<uint64_t>(ch & 0xFFFFu);
    hash *= 1099511628211ull;
  }
  return hash;
}

std::vector<AttachedMonitor> EnumerateAttachedMonitors() {
  std::vector<AttachedMonitor> out;
  EnumCtx ctx{&out};
  EnumDisplayMonitors(nullptr, nullptr, EnumProc, reinterpret_cast<LPARAM>(&ctx));
  std::sort(out.begin(), out.end(),
            [](const AttachedMonitor& a, const AttachedMonitor& b) {
              return a.deviceName < b.deviceName;
            });
  return out;
}

HMONITOR MonitorFromTaskbarRect(const RECT& rc) {
  const POINT pt{(rc.left + rc.right) / 2, (rc.top + rc.bottom) / 2};
  return MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);
}

bool MonitorDeviceNameFromRect(const RECT& rc, std::wstring& outDeviceName) {
  HMONITOR mon = MonitorFromTaskbarRect(rc);
  if (!mon) return false;
  MONITORINFOEXW mi{};
  mi.cbSize = sizeof(mi);
  if (!GetMonitorInfoW(mon, &mi)) return false;
  outDeviceName = mi.szDevice;
  return true;
}

MonitorAppearance ResolveMonitorAppearance(const Settings& settings,
                                           std::wstring_view deviceName) {
  for (const auto& entry : settings.monitorSettings) {
    if (entry.deviceName == deviceName) return entry.appearance;
  }
  MonitorAppearance a;
  a.theme = settings.theme;
  a.intensity = settings.intensity;
  return a;
}

bool IsMonitorDisabled(const Settings& settings, std::wstring_view deviceName) {
  for (const auto& name : settings.disabledMonitors) {
    if (name == deviceName) return true;
  }
  return false;
}

void SetMonitorDisabled(Settings& settings, std::wstring_view deviceName, bool disabled) {
  auto& list = settings.disabledMonitors;
  const auto it =
      std::find_if(list.begin(), list.end(),
                   [&](const std::wstring& n) { return n == deviceName; });
  if (disabled) {
    if (it == list.end()) list.emplace_back(deviceName);
  } else if (it != list.end()) {
    list.erase(it);
  }
}

void UpsertMonitorAppearance(Settings& settings, std::wstring_view deviceName,
                             const MonitorAppearance& appearance) {
  for (auto& entry : settings.monitorSettings) {
    if (entry.deviceName == deviceName) {
      entry.appearance = appearance;
      return;
    }
  }
  MonitorAppearanceEntry entry;
  entry.deviceName = std::wstring(deviceName);
  entry.appearance = appearance;
  settings.monitorSettings.push_back(std::move(entry));
}

void ApplyAppearanceToAllAttached(Settings& settings, const MonitorAppearance& appearance) {
  settings.theme = appearance.theme;
  settings.intensity = appearance.intensity;
  const auto attached = EnumerateAttachedMonitors();
  for (const auto& mon : attached) {
    UpsertMonitorAppearance(settings, mon.deviceName, appearance);
  }
}

}  // namespace phostide
