// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:ignore \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a scalar assignment from an inexactly overlapping object
// (ub:expr.assign.overlap) configured to "ignore": no handler call,
// and execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-ignore.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
union U {
  int i;
  short s;
};
__attribute__((noinline)) int assign(U *u) {
  u->i = u->s;
  return u->i;
}
int main() {
  U u;
  u.s = 1;
  int r = assign(&u);
  std::printf("RESULT=%d\n", r == 1);
  return 0;
}
// CHECK-NOT: VIOL
// CHECK: RESULT=1
