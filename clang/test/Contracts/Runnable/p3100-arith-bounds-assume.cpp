// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: pointer arithmetic out of the bounds of an array of known bound
// (ub:expr.add.out.of.bounds.known.arithmetic) left at the default "assume":
// no check is emitted, and the pointer is formed four elements on.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-assume.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
int arr[3] = {10, 20, 30};
__attribute__((noinline)) int *add(int i) { return arr + i; }
int main() {
  std::printf("RESULT=%d\n", (int)(add(4) - arr));
  return 0;
}
// CHECK-NOT: VIOL
// CHECK: RESULT=4
