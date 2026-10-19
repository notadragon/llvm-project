// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.static.cast.enum.outside.range.integral:observe,ub:expr.static.cast.enum.outside.range.floating:ignore \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.static.cast.enum.outside.range:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s --check-prefix=PARENT
// P3100: the enumeration-range check reports
// ub:expr.static.cast.enum.outside.range.integral for an integral source and
// .floating for a floating-point one, so the two are configured independently:
// here the integral cast is observed (the handler runs, and the result is 0)
// and the floating-point cast ignored (no handler, but the result is 0 too,
// where the default "assume" would leave 5).  The parent group still selects
// both.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-enum-cast-split.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL [%s]\n", v.comment());
  std::fflush(stdout);
}
enum E { A, B, C }; // [dcl.enum] value range [0,3]
__attribute__((noinline)) E from_int(int v) { return static_cast<E>(v); }
__attribute__((noinline)) E from_double(double v) { return static_cast<E>(v); }
int main() {
  int i = from_int(5);
  int d = from_double(5.0);
  std::printf("RESULT i=%d d=%d\n", i, d);
  return 0;
}
// CHECK: VIOL [enumeration value out of range]
// CHECK-NEXT: RESULT i=0 d=0
// PARENT: VIOL [enumeration value out of range]
// PARENT-NEXT: VIOL [enumeration value out of range]
// PARENT-NEXT: RESULT i=0 d=0
