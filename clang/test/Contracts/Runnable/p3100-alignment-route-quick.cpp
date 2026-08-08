// P3100 Task 4.1 (UBSan runtime routing):
// -fsanitize-semantic=alignment:quick_enforce terminates WITHOUT calling the
// handler and WITHOUT any output -- the routed runtime Die()s silently.
// quick_enforce needs no -fcontracts-p4298.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=alignment -fsanitize-semantic=alignment:quick_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("handler ran\n"); // must NOT run for quick_enforce
  std::fflush(stdout);
}

int __attribute__((noinline)) load(int *p) { return *p; }

int main() {
  alignas(int) char buf[8];
  int *p = reinterpret_cast<int *>(buf + 1);
  volatile int sink = load(p);
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// quick_enforce = silent terminate: no handler, no stock report, no "survived".
// CHECK-NOT: handler ran
// CHECK-NOT: runtime error
// CHECK-NOT: survived
