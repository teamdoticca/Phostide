#pragma once

// Minimal self-registering test harness (no external framework). Each test TU
// #includes the production .cpp it covers so file-local helpers are reachable.

#include <cstdio>
#include <vector>

namespace phostide_tests {

struct TestCase {
  const char* name;
  void (*fn)();
};

inline std::vector<TestCase>& Registry() {
  static std::vector<TestCase> tests;
  return tests;
}

inline int& Failures() {
  static int failures = 0;
  return failures;
}

}  // namespace phostide_tests

#define PT_CONCAT_(a, b) a##b
#define PT_CONCAT(a, b) PT_CONCAT_(a, b)

#define TEST(name)                                                  \
  static void name();                                               \
  static const bool PT_CONCAT(name, _registered) =                  \
      (phostide_tests::Registry().push_back({#name, &name}), true); \
  static void name()

#define CHECK(cond)                                                     \
  do {                                                                  \
    if (!(cond)) {                                                      \
      std::printf("    FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
      ++phostide_tests::Failures();                                     \
    }                                                                   \
  } while (0)
