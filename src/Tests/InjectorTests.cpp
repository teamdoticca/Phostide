// TAP inject timing: never hook a just-started Explorer (ixde fails for its lifetime).
#include "../Host/Injector.cpp"
#include "TestHarness.h"

using namespace phostide;

TEST(ExplorerSettleWait_WaitsOutAYoungExplorer) {
  // Setup update: its own shell restart settles 8 s, then starts Phostide.
  CHECK(ExplorerSettleWaitMs(10000) == 5000);
  CHECK(ExplorerSettleWaitMs(0) == kExplorerSettleMs);
}

TEST(ExplorerSettleWait_NoWaitOnceSettled) {
  CHECK(ExplorerSettleWaitMs(kExplorerSettleMs) == 0);
  CHECK(ExplorerSettleWaitMs(3600000) == 0);
  CHECK(ExplorerSettleWaitMs(UINT64_MAX) == 0);  // age unknown: hook as before
}

TEST(ProcessAgeMs_ReadsThisProcess) {
  const uint64_t age = ProcessAgeMs(GetCurrentProcessId());
  CHECK(age != UINT64_MAX);
  CHECK(age < 10ull * 60 * 1000);
  CHECK(ProcessAgeMs(0xFFFFFFF0u) == UINT64_MAX);
}
