// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at a scalar assignment from an
// inexactly overlapping object (ub:expr.assign.overlap) under "enforce": the
// exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-throw.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
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
  try {
    U u;
    u.s = 1;
    int r = assign(&u);
    std::printf("RESULT=%d\n", r == 1);
  } catch (Boom &) {
    std::printf("CAUGHT\n");
    std::fflush(stdout);
  }
  std::printf("SURVIVED\n");
  return 0;
}
// CHECK: VIOL sem=3
// CHECK-NEXT: CAUGHT
// CHECK-NEXT: SURVIVED
