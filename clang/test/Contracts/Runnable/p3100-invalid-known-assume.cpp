// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: an indirection past the end of an array of known bound
// (ub:expr.unary.dereference.invalid.known) left at the default "assume": no
// check is emitted, and the one-past address is formed.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-known-assume.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
int arr[3] = {10, 20, 30};
__attribute__((noinline)) int *addr(int i) { return &arr[i]; }
int main() {
  std::printf("RESULT=%d\n", (int)(addr(3) - arr));
  return 0;
}
// CHECK-NOT: VIOL
// CHECK: RESULT=3
