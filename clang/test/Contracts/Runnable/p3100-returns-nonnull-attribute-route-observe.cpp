// P3100 (UBSan runtime routing): the returns-nonnull-attribute check
// (-fsanitize=returns-nonnull-attribute) routes to the contract-violation
// handler when a function marked __attribute__((returns_nonnull)) returns a null
// pointer.  With the check resolved to noexcept_observe the handler runs
// (kind=7, semantic=6) and the program CONTINUES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=returns-nonnull-attribute \
// RUN:   -fsanitize-recover=returns-nonnull-attribute %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

__attribute__((returns_nonnull)) int *__attribute__((noinline)) g() {
  int *volatile q = nullptr;
  return q; // returns-nonnull-attribute: null returned from returns_nonnull
}

int main() {
  volatile int *r = g();
  (void)r;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
