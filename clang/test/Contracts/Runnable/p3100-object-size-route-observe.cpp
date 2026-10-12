// P3100 (UBSan runtime routing): the object-size check
// (-fsanitize=object-size) routes to the contract-violation handler.  With the
// check resolved to noexcept_observe the handler runs (kind=7, semantic=6) and
// the program CONTINUES.  object-size needs __builtin_object_size, emitted only
// at -O1+, so this is compiled at -O2.

// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=object-size -fsanitize-recover=object-size \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

alignas(int) static char buf[1]; // aligned, but too small for an int

int main() {
  int *p = reinterpret_cast<int *>(&buf[0]);
  volatile int sink = *p; // object-size: 4-byte access on a 1-byte object
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
