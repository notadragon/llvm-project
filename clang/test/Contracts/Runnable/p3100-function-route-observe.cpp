// P3100 (UBSan runtime routing): the function check
// (-fsanitize=function) routes to the contract-violation handler when a function
// is called through a pointer of the wrong type.  -fsanitize=function is
// Clang-only (GCC has no such check), so this routed check has no GCC
// counterpart.  With the check resolved to noexcept_observe the handler runs
// (kind=7, semantic=6) and the program CONTINUES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=function -fsanitize-recover=function %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

void target() {}

int main() {
  // Call target() through a pointer of the wrong function type.
  auto p = reinterpret_cast<int (*)(int)>(&target);
  volatile int x = p(5); // function: type mismatch at the indirect call
  (void)x;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
