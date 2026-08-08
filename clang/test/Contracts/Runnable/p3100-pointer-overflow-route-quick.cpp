// P3100 Task 4.1 (UBSan runtime routing):
// -fsanitize-semantic=pointer-overflow:quick_enforce terminates WITHOUT calling
// the handler and WITHOUT any output.  quick_enforce needs no -fcontracts-p4298.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=pointer-overflow \
// RUN:   -fsanitize-semantic=pointer-overflow:quick_enforce %libcxx_flags -o %t \
// RUN:   && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("handler ran\n"); // must NOT run for quick_enforce
  std::fflush(stdout);
}

char *volatile sink;

char *__attribute__((noinline)) add(char *p, unsigned long i) { return p + i; }

int main() {
  char *n = nullptr;
  sink = add(n, 1);
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK-NOT: handler ran
// CHECK-NOT: runtime error
// CHECK-NOT: survived
