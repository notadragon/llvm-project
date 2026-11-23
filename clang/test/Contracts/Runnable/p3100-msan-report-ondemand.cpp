// P3100 RF (Clang-only): on the routed path MSan emits NOTHING; the handler owns
// all output and retrieves the sanitizer's report on demand via
// contract_violation::report() (requires -fcontracts-p4301): the error, its
// stack, and with -fsanitize-memory-track-origins where the value came from.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=memory \
// RUN:   -fsanitize-semantic=memory:noexcept_observe %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s --check-prefixes=CHECK,NOORIGIN
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontracts-p4301 -fsanitize=memory -fsanitize-memory-track-origins \
// RUN:   -fsanitize-semantic=memory:noexcept_observe %libcxx_flags -o %t2 \
// RUN:   && %t2 2>&1 | FileCheck %s --check-prefixes=CHECK,ORIGIN

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

// The report() text is the first output: the error, then its stack, then
// (with origin tracking) the origin.  MSan itself prints nothing.
// CHECK-NOT: WARNING: MemorySanitizer
// CHECK: report=MemorySanitizer: use-of-uninitialized-value
// CHECK-NEXT: #0 0x
// ORIGIN: Uninitialized value was created by an allocation of '{{.*}}y{{.*}}' in the stack frame
// NOORIGIN-NOT: Uninitialized value was created
// CHECK: survived
