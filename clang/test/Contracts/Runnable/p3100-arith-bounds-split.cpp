// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: configuring only the .arithmetic leaf of
// ub:expr.add.out.of.bounds.known checks a + i, and leaves the subscript a[i]
// (the .subscript leaf) at the default "assume".
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-split.C)

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
  int s = &a[vi] - a;   // 5: not checked
  int p = (a + vi) - a; // caught -> 0
  std::printf("RESULT s=%d p=%d viols=%d\n", s, p, viols);
  return 0;
}
// CHECK: RESULT s=5 p=0 viols=1
