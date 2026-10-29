// P3100 full-coverage UBSan routing (Clang): the null check routes to the handler.
// Null dereference has no defined fallback:  noexcept_observe runs the handler once (kind 7, semantic 6), then the
// no-fallback violation terminates/faults.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298  \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(), (int)v.semantic());
  std::fflush(stdout);
}

int main() { int *volatile p = nullptr; volatile int r = *p; (void)r; std::printf("survived\n"); std::fflush(stdout); }

// CHECK: handler kind=7 semantic=6
// CHECK-NOT: survived
