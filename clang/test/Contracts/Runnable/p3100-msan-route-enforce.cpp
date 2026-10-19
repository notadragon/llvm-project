// P3100 (Clang-only): with -fcontracts-p4298 and
// -fsanitize-semantic=memory:noexcept_enforce the routed memory check resolves
// to noexcept_enforce: the handler runs, then the program terminates (MSan emits
// the __msan_warning_noreturn entry).  Nonzero exit (wrapped with `not`).

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=memory -fsanitize-semantic=memory:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
                   (int)v.semantic());
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

// Handler runs (kind 7, noexcept_enforce = 7), then terminate.
// CHECK: handler kind=7 semantic=7
// CHECK-NOT: NOTREACHED
