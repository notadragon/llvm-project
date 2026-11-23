// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.subscript:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: the subscript check is the .subscript leaf of
// ub:expr.add.out.of.bounds.known.  Configuring only that leaf checks a[i],
// and leaves the pointer arithmetic a + i (the .arithmetic leaf) at the
// default "assume".
// (GCC mirror: g++.dg/contracts/cpp26/p3100-array-bounds-split-subscript.C)

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
  int p = (a + vi) - a; // 5: not checked
  std::printf("RESULT s=%d p=%d viols=%d\n", s, p, viols);
  return 0;
}
// CHECK: RESULT s=0 p=5 viols=1
