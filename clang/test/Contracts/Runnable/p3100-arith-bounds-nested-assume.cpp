// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: nested pointer arithmetic (a + i) + j and the address of a subscript
// (a + 1)[i] past the end of an array of known bound, with the bounds and
// indirection checks left at the default "assume": no check is emitted and
// the pointers are computed as written.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-nested-assume.C)

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
__attribute__((noinline)) int *subaddr(int i) { return &(a + 1)[i]; }
int main() {
  int r = (int)(plus(2, 3) - a) + (int)(subaddr(4) - a);
  std::printf("RESULT=%d\n", r);
  return 0;
}
// CHECK-NOT: VIOL
// CHECK: RESULT=10
