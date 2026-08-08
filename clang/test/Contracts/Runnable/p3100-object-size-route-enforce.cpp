// P3100 Task 4.1 (UBSan runtime routing): with object-size resolved to
// noexcept_enforce the handler runs (kind=7, semantic=7) then the program
// TERMINATES.  Compiled at -O2 (object-size only instruments at -O1+).

// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontracts-p4298 -fsanitize=object-size \
// RUN:   -fsanitize-semantic=object-size:noexcept_enforce %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

alignas(int) static char buf[1];

int main() {
  int *p = reinterpret_cast<int *>(&buf[0]);
  volatile int sink = *p;
  (void)sink;
  std::printf("survived\n"); // must NOT be reached
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
