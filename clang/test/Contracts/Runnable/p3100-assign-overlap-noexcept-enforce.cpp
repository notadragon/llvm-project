// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:noexcept_enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a scalar assignment from an inexactly overlapping object
// (ub:expr.assign.overlap) configured to "noexcept_enforce": the
// handler runs (kind=7, sem=7), then the program terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-noexcept-enforce.C)

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
// CHECK: VIOL kind=7 sem=7 comment=[assignment from an overlapping object]
// CHECK-NOT: RESULT
