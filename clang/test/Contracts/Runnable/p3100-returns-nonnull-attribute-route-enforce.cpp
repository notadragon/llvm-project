// P3100 Task 4.1 (UBSan runtime routing): with returns-nonnull-attribute
// resolved to noexcept_enforce the handler runs (kind=7, semantic=7) then the
// program TERMINATES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=returns-nonnull-attribute \
// RUN:   -fsanitize-semantic=returns-nonnull-attribute:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

__attribute__((returns_nonnull)) int *__attribute__((noinline)) g() {
  int *volatile q = nullptr;
  return q;
}

int main() {
  volatile int *r = g();
  (void)r;
  std::printf("survived\n"); // must NOT be reached
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
