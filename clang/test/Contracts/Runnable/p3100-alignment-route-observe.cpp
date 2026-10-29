// P3100 (UBSan runtime routing): the alignment check
// (-fsanitize=alignment) routes to the contract-violation handler.  With the
// check resolved to noexcept_observe (via -fsanitize-recover=alignment +
// -fcontracts-p4298) the handler runs (kind=7, semantic=6) and the program
// CONTINUES.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=alignment -fsanitize-recover=alignment %libcxx_flags -o %t \
// RUN:   && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

int __attribute__((noinline)) load(int *p) {
  return *p; // alignment: *p read through a misaligned pointer.
}

int main() {
  alignas(int) char buf[8];
  int *p = reinterpret_cast<int *>(buf + 1); // one byte off -> misaligned
  volatile int sink = load(p);
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
