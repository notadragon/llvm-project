// P3100 full-coverage UBSan routing (Clang): the enum check routes to the
// contract-violation handler; noexcept_observe runs the handler (kind 7,
// semantic 6) and the program continues.

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298  \
// RUN:   -fsanitize=enum -fsanitize-semantic=enum:noexcept_observe %libcxx_flags \
// RUN:   -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(), (int)v.semantic());
  std::fflush(stdout);
}

enum E { A, B }; int main() { int i = 7; E volatile e = *reinterpret_cast<E *>(&i); (void)e; std::printf("survived\n"); std::fflush(stdout); }

// CHECK: handler kind=7 semantic=6
// CHECK: survived
