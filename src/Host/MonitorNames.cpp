#include "MonitorNames.h"

#include <windows.h>

#include <cwctype>
#include <vector>

namespace phostide {
namespace {

bool IsBuiltInPanel(DISPLAYCONFIG_VIDEO_OUTPUT_TECHNOLOGY tech) {
  return tech == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INTERNAL ||
         tech == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_DISPLAYPORT_EMBEDDED ||
         tech == DISPLAYCONFIG_OUTPUT_TECHNOLOGY_UDI_EMBEDDED;
}

bool EqualsNoCase(std::wstring_view a, std::wstring_view b) {
  if (a.size() != b.size()) return false;
  for (size_t i = 0; i < a.size(); ++i) {
    if (std::towlower(a[i]) != std::towlower(b[i])) return false;
  }
  return true;
}

}  // namespace

std::wstring CleanMonitorFriendlyName(std::wstring_view raw) {
  size_t begin = 0;
  size_t end = raw.size();
  while (begin < end && (std::iswspace(raw[begin]) || raw[begin] == L'\0')) ++begin;
  while (end > begin && (std::iswspace(raw[end - 1]) || raw[end - 1] == L'\0')) --end;
  const std::wstring_view name = raw.substr(begin, end - begin);
  if (name.empty()) return {};
  for (std::wstring_view placeholder :
       {L"Generic PnP Monitor", L"Generic Non-PnP Monitor", L"Default Monitor",
        L"Generic Monitor"}) {
    if (EqualsNoCase(name, placeholder)) return {};
  }
  return std::wstring(name);
}

std::map<std::wstring, std::wstring> QueryMonitorFriendlyNames() {
  std::map<std::wstring, std::wstring> names;

  std::vector<DISPLAYCONFIG_PATH_INFO> paths;
  std::vector<DISPLAYCONFIG_MODE_INFO> modes;
  LONG rc = ERROR_INSUFFICIENT_BUFFER;
  // The topology can change between the size query and the fetch; retry a few times.
  for (int attempt = 0; attempt < 4 && rc == ERROR_INSUFFICIENT_BUFFER; ++attempt) {
    UINT32 pathCount = 0;
    UINT32 modeCount = 0;
    if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &pathCount, &modeCount) !=
        ERROR_SUCCESS) {
      return names;
    }
    paths.resize(pathCount);
    modes.resize(modeCount);
    rc = QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &pathCount, paths.data(), &modeCount,
                            modes.data(), nullptr);
    paths.resize(pathCount);
    modes.resize(modeCount);
  }
  if (rc != ERROR_SUCCESS) return names;

  for (const auto& path : paths) {
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
    source.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
    source.header.size = sizeof(source);
    source.header.adapterId = path.sourceInfo.adapterId;
    source.header.id = path.sourceInfo.id;
    if (DisplayConfigGetDeviceInfo(&source.header) != ERROR_SUCCESS) continue;

    DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
    target.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
    target.header.size = sizeof(target);
    target.header.adapterId = path.targetInfo.adapterId;
    target.header.id = path.targetInfo.id;
    if (DisplayConfigGetDeviceInfo(&target.header) != ERROR_SUCCESS) continue;

    std::wstring name = CleanMonitorFriendlyName(target.monitorFriendlyDeviceName);
    if (name.empty() && IsBuiltInPanel(target.outputTechnology)) name = L"Built-in display";
    if (name.empty()) continue;

    // Cloned outputs share one GDI source: keep the first name (rows are per source).
    names.emplace(source.viewGdiDeviceName, std::move(name));
  }
  return names;
}

}  // namespace phostide
