#include "TestHarness.h"

int main() {
  int failedTests = 0;
  for (const auto& t : phostide_tests::Registry()) {
    const int before = phostide_tests::Failures();
    t.fn();
    const bool ok = phostide_tests::Failures() == before;
    if (!ok) ++failedTests;
    std::printf("%s  %s\n", ok ? "PASS" : "FAIL", t.name);
  }
  std::printf("\n%zu tests, %d failed\n", phostide_tests::Registry().size(), failedTests);
  return failedTests == 0 ? 0 : 1;
}
