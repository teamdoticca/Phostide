#pragma once

#include <map>
#include <string>
#include <string_view>

namespace phostide {

// Model names for attached displays, keyed by GDI device name (\\.\DISPLAY1).
// Source: Windows DisplayConfig target names, which come from the monitor's EDID
// (e.g. "LG 27GR95QE"). Laptop panels without an EDID name become "Built-in display".
// Displays with no usable name are simply missing from the map.
std::map<std::wstring, std::wstring> QueryMonitorFriendlyNames();

// Trims whitespace and drops placeholder names Windows uses when the EDID has none
// ("Generic PnP Monitor", ...). Returns empty when nothing useful is left.
std::wstring CleanMonitorFriendlyName(std::wstring_view raw);

}  // namespace phostide
