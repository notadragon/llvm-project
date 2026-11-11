// P3100 (Clang-only): under -fcontracts-p3100 a MemorySanitizer-detected
// use-of-uninitialized-value is routed to the contract-violation handler.  With
// -fcontracts-p4298 and -fsanitize-semantic=memory:noexcept_observe the memory
// check resolves to noexcept_observe: the handler runs (kind=7, semantic=6) and
// the program CONTINUES (MSan emits the recoverable __msan_warning entry).
// Library-free trigger (a plain indeterminate int), so no MSan-instrumented
// libc++ is needed.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
                   (int)v.semantic());
  std::fflush(stdout);
}

__attribute__((noinline)) int trigger(int x) {
  int y [[indeterminate]];
  return y + (x - x); // reading an indeterminate value: UB
}

int main() {
  volatile int seed = 0;
  int v = trigger(seed);
  // Using the indeterminate value in a branch is where MSan checks it.
  if (v)
    std::printf("a\n");
  else
    std::printf("b\n");
  // noexcept_observe = report + continue: we reach here after the violation.
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// The handler runs (kind 7, noexcept_observe = 6); nothing leaks before it.
// CHECK: handler kind=7 semantic=6
// CHECK: survived
