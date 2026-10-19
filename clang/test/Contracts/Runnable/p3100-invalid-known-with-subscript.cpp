// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known:observe,ub:expr.unary.dereference.invalid.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: with both the bounds checks (ub:expr.add.out.of.bounds.known: the
// subscript and arithmetic leaves) and the past-the-end indirection check
// (ub:expr.unary.dereference.invalid.known) observed, &a[N + 2] and
// *(a + N + 2) are reported once, by the bounds check, which redirects the
// index before the indirection check sees it; &a[N] and *(a + N) are reported
// by the indirection check alone.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-known-with-subscript.C,
// for the subscript)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL [%s]\n", v.comment());
  std::fflush(stdout);
}
int a[3] = {10, 20, 30};
volatile int vi;
int main() {
  vi = 5; int r1 = &a[vi] - a;
  std::printf("R1=%d\n", r1);
  vi = 3; int r2 = &a[vi] - a;
  std::printf("R2=%d\n", r2);
  vi = 5; int r3 = *(a + vi);
  std::printf("R3=%d\n", r3);
  vi = 3; int r4 = *(a + vi);
  std::printf("R4=%d\n", r4);
  return 0;
}
// CHECK: VIOL [array subscript out of bounds]
// CHECK-NEXT: R1=0
// CHECK-NEXT: VIOL [indirection past the end of an array]
// CHECK-NEXT: R2=0
// CHECK-NEXT: VIOL [pointer arithmetic out of bounds]
// CHECK-NEXT: R3=10
// CHECK-NEXT: VIOL [indirection past the end of an array]
// CHECK-NEXT: R4=10
