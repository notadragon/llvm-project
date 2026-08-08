// P3100 Task 4.1 (UBSan runtime routing): with the alignment check resolved to
// noexcept_enforce the handler runs (kind=7, semantic=7) and the program then
// TERMINATES.  The terminating semantic rides the sanitizer's NON-recovering
// (abort) code path.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=alignment -fsanitize-semantic=alignment:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

int __attribute__((noinline)) load(int *p) { return *p; }

int main() {
  alignas(int) char buf[8];
  int *p = reinterpret_cast<int *>(buf + 1);
  volatile int sink = load(p);
  (void)sink;
  std::printf("survived\n"); // must NOT be reached
  std::fflush(stdout);
  return 0;
}

// CHECK: handler kind=7 semantic=7
// CHECK-NOT: survived
