// P3100 (UBSan runtime routing): with pointer-overflow resolved to
// noexcept_enforce the handler runs (kind=7, semantic=7) then the program
// TERMINATES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=pointer-overflow \
// RUN:   -fsanitize-semantic=pointer-overflow:noexcept_enforce %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

char *volatile sink;

char *__attribute__((noinline)) add(char *p, unsigned long i) { return p + i; }

int main() {
  char *n = nullptr;
  sink = add(n, 1);
  std::printf("survived\n"); // must NOT be reached
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
