// Monitor names shown in Settings > Monitors / Welcome (EDID via DisplayConfig).
#include "../Host/MonitorNames.cpp"
#include "TestHarness.h"

using namespace phostide;

TEST(CleanMonitorFriendlyName_KeepsModelNames) {
  CHECK(CleanMonitorFriendlyName(L"LG 27GR95QE") == L"LG 27GR95QE");
  CHECK(CleanMonitorFriendlyName(L"  DELL AW3423DWF \0") == L"DELL AW3423DWF");
}

TEST(CleanMonitorFriendlyName_DropsPlaceholders) {
  CHECK(CleanMonitorFriendlyName(L"").empty());
  CHECK(CleanMonitorFriendlyName(L"   ").empty());
  CHECK(CleanMonitorFriendlyName(L"Generic PnP Monitor").empty());
  CHECK(CleanMonitorFriendlyName(L"generic non-pnp monitor").empty());
}

TEST(QueryMonitorFriendlyNames_DoesNotThrow) {
  // Machine-dependent (CI runners may have no display): only prove the call is safe
  // and every key looks like a GDI device name.
  const auto names = QueryMonitorFriendlyNames();
  for (const auto& [device, name] : names) {
    CHECK(device.rfind(L"\\\\.\\", 0) == 0);
    CHECK(!name.empty());
  }
}
