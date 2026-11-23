// P3100 full-coverage UBSan routing (Clang-only): the implicit-conversion group
// (a well-defined-but-flagged conversion, not core UB) routes to the handler;
// noexcept_observe runs the handler (kind 7, semantic 6) and continues.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=implicit-conversion \
// RUN:   -fsanitize-semantic=implicit-conversion:noexcept_observe %libcxx_flags \
// RUN:   -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(), (int)v.semantic());
  std::fflush(stdout);
}

int main() {
  volatile int x = 300;
  volatile signed char c = x; // implicit truncation 300 -> signed char
  (void)c;
  std::printf("survived\n");
  std::fflush(stdout);
}

// CHECK: handler kind=7 semantic=6
// CHECK: survived
