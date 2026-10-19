// P3100 full-coverage UBSan routing (Clang): the unreachable check routes to the handler.
// There is nowhere to continue:  noexcept_observe runs the handler once (kind 7, semantic 6), then the
// no-fallback violation terminates/faults.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298  \
// RUN:   -fsanitize=unreachable -fsanitize-semantic=unreachable:noexcept_observe %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(), (int)v.semantic());
  std::fflush(stdout);
}

int main() { volatile int z = 0; if (z) return 0; __builtin_unreachable(); }

// CHECK: handler kind=7 semantic=6
// CHECK-NOT: survived
