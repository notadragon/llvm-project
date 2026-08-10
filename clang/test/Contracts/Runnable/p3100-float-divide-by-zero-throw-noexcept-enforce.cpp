// P3100 x P4298 sanitizer-routed path (regression): a THROWING contract-
// violation handler under noexcept_enforce must TERMINATE the program, not
// escape into the sanitizer runtime's non-unwindable C frames.  Routes the
// UBSan float-divide-by-zero check to the handler and has the handler throw;
// the routed-path terminate barrier (__contract_dispatch_core_noexcept, reached
// via __cxa_contract_violation_sanitizer) must catch the throw and call
// std::terminate.
//
// LINKAGE: the barrier is defined in libc++ (contracts_abi.cpp) and referenced
// as a weak undef by the shared libcontracts.so; libc++ DT_NEEDEDs libcontracts,
// so the weak ref resolves to libc++'s definition at load time with no -Wl,-u
// needed (unlike GCC's static-archive libstdc++exp).  A routed-only TU therefore
// still terminates correctly.
//
// A set_terminate marker prints TERMINATED and exits 0 (the correct path);
// main wraps the trigger in try/catch and prints ESCAPED if the exception ever
// escapes the barrier (the bug), or survived if the check never fired.
//
// (GCC mirror: g++.dg/ubsan/p3100-float-divide-by-zero-throw-noexcept-enforce.C.)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=float-divide-by-zero \
// RUN:   -fsanitize-semantic=float-divide-by-zero:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>

struct E {};
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
  throw E{};                     // must be caught by the barrier -> terminate
}

static volatile double vzero = 0.0;
__attribute__((noinline)) static double trig() { return 6.0 / vzero; }

int main() {
  std::set_terminate([] { std::printf("TERMINATED\n"); std::fflush(stdout);
                          std::_Exit(0); });
  try {
    volatile double r = trig(); (void)r;
    std::printf("survived\n");             // bug: check never fired / observe
  } catch (...) {
    std::printf("ESCAPED\n");              // bug: exception escaped the barrier
  }
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK: TERMINATED
// CHECK-NOT: ESCAPED
// CHECK-NOT: survived
