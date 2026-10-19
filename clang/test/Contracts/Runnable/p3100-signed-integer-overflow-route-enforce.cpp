// P3100 full-coverage UBSan routing (Clang): noexcept_enforce runs the handler
// (kind 7, semantic 7) then terminates -- representative of the terminating
// direction for the full-coverage batch.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=signed-integer-overflow \
// RUN:   -fsanitize-semantic=signed-integer-overflow:noexcept_enforce %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(), (int)v.semantic());
  std::fflush(stdout);
}

int main() {
  volatile int a = 2147483647, b = 1;
  volatile int r = a + b;
  (void)r;
  std::printf("survived\n");
  std::fflush(stdout);
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
