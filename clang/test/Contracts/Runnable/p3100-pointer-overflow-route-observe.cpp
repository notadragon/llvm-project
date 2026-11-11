// P3100 (UBSan runtime routing): the pointer-overflow check
// (-fsanitize=pointer-overflow) routes to the contract-violation handler.  With
// the check resolved to noexcept_observe the handler runs (kind=7, semantic=6)
// and the program CONTINUES.  (Clang flags applying a non-zero offset to a null
// pointer as pointer-overflow; the GCC test uses a wrapping add, which is the
// sub-case GCC instruments -- the two compilers cover different sub-cases.)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=pointer-overflow -fsanitize-recover=pointer-overflow \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

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
  sink = add(n, 1); // pointer-overflow: non-zero offset applied to null pointer
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
