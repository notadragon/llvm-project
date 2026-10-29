// P3100 (Clang-only): -fsanitize-semantic=memory:quick_enforce terminates on the
// first use-of-uninitialized-value WITHOUT entering the handler and WITHOUT
// printing anything (the routed sanitizer emits nothing).  Accepted without
// -fcontracts-p4298.  Nonzero exit (wrapped with `not`); no output.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:quick_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>

// Must NOT run under quick_enforce.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d\n", (int)v.kind());
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
  std::printf("NOTREACHED\n");
  std::fflush(stdout);
  return 0;
}

// quick_enforce = silent terminate: no handler, no output.
// CHECK-NOT: handler
// CHECK-NOT: NOTREACHED
// CHECK-NOT: MemorySanitizer
