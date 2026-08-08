// P3100 RF (Clang-only): on the routed path MSan emits NOTHING; the handler owns
// all output and retrieves the sanitizer's description on demand via
// contract_violation::report() (requires -fcontracts-p4301).  v1: a concise
// description; full multi-line capture is a documented follow-up.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=memory \
// RUN:   -fsanitize-semantic=memory:noexcept_observe %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  const char *r = v.report();
  std::printf("report=%s\n", r ? r : "(null)");
  std::fflush(stdout);
}

__attribute__((noinline)) int trigger(int x) {
  int y [[indeterminate]];
  return y + (x - x);
}

int main() {
  volatile int seed = 0;
  int v = trigger(seed);
  if (v)
    std::printf("a\n");
  else
    std::printf("b\n");
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// The report() text (concise v1 description) is the first output.
// CHECK: report=MemorySanitizer: use-of-uninitialized-value
// CHECK: survived
