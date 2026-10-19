// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: the parent group ub:expr.add.out.of.bounds.known still selects both
// of its leaves, .subscript and .arithmetic.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-split-parent.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
int a[3];
volatile int vi;
int main() {
  vi = 5;
  int s = &a[vi] - a;   // caught -> 0
  int p = (a + vi) - a; // caught -> 0
  std::printf("RESULT s=%d p=%d viols=%d\n", s, p, viols);
  return 0;
}
// CHECK: RESULT s=0 p=0 viols=2
