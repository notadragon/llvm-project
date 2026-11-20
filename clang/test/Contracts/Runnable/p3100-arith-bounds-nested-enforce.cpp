// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: nested pointer arithmetic (a + i) + j past the end of an array of
// known bound (ub:expr.add.out.of.bounds.known.arithmetic) configured to
// "enforce": the handler runs (kind=7, sem=3), then the program terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-nested-enforce.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
int a[4];
__attribute__((noinline)) int *plus(int i, int j) { return (a + i) + j; }
int main() {
  int *p = plus(2, 3);
  std::printf("RESULT=%d\n", (int)(p - a));
  return 0;
}
// CHECK: VIOL kind=7 sem=3 comment=[pointer arithmetic out of bounds]
// CHECK-NOT: RESULT
