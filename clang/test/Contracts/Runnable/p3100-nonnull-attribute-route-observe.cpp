// P3100 Task 4.1 (UBSan runtime routing): the nonnull-attribute check
// (-fsanitize=nonnull-attribute) routes to the contract-violation handler when a
// null pointer is passed to a parameter marked __attribute__((nonnull)).  With
// the check resolved to noexcept_observe the handler runs (kind=7, semantic=6)
// and the program CONTINUES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=nonnull-attribute -fsanitize-recover=nonnull-attribute \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

// nonnull attribute is on the function: argument 1 must be non-null.
void __attribute__((noinline, nonnull(1))) g(int *p) { (void)p; }

int main() {
  int *volatile q = nullptr;
  g(q); // nonnull-attribute: null passed to a nonnull parameter
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
