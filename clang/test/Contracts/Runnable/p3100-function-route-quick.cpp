// P3100 (UBSan runtime routing):
// -fsanitize-semantic=function:quick_enforce terminates WITHOUT calling the
// handler and WITHOUT any output.  -fsanitize=function is Clang-only.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=function -fsanitize-semantic=function:quick_enforce \
// RUN:   %libcxx_flags -o %t && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("handler ran\n"); // must NOT run for quick_enforce
  std::fflush(stdout);
}

void target() {}

int main() {
  auto p = reinterpret_cast<int (*)(int)>(&target);
  volatile int x = p(5);
  (void)x;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK-NOT: handler ran
// CHECK-NOT: runtime error
// CHECK-NOT: survived
