#pragma once

#include "PhostideCommon.h"

#include <vector>

#include <windows.h>

namespace phostide {

struct AttachedMonitor {
  std::wstring deviceName;  // MONITORINFOEX.szDevice, e.g. \\.\DISPLAY1
  uint64_t deviceHash = 0;
  RECT rc{};
  int width = 0;
  int height = 0;
  bool primary = false;
};

// FNV-1a 64-bit over UTF-16 device name (null-terminated not included).
uint64_t HashMonitorDeviceName(std::wstring_view deviceName);

// Enumerate attached monitors sorted by deviceName (stable slot order).
std::vector<AttachedMonitor> EnumerateAttachedMonitors();

HMONITOR MonitorFromTaskbarRect(const RECT& rc);

bool MonitorDeviceNameFromRect(const RECT& rc, std::wstring& outDeviceName);

MonitorAppearance ResolveMonitorAppearance(const Settings& settings,
                                           std::wstring_view deviceName);

bool IsMonitorDisabled(const Settings& settings, std::wstring_view deviceName);

void SetMonitorDisabled(Settings& settings, std::wstring_view deviceName, bool disabled);

void UpsertMonitorAppearance(Settings& settings, std::wstring_view deviceName,
                             const MonitorAppearance& appearance);

void ApplyAppearanceToAllAttached(Settings& settings, const MonitorAppearance& appearance);

}  // namespace phostide
