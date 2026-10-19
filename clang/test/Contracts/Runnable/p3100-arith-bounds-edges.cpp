// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: pointer arithmetic on an array of known bound
// (ub:expr.add.out.of.bounds.known.arithmetic) under "observe".  The result
// must lie in [0, N]: the one-past pointer and a negative offset to a non-
// negative position are fine; offsets past N, below 0, an unsigned offset
// standing for -1, the reversed operand order, a member array and a row of a
// two-dimensional array are caught, each continuing with the offset 0.
// Arithmetic on a pointer variable, and on a trailing array that may be a
// flexible array member, has no known bound and is not checked.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
int a[3];
int m[2][3];
struct S { int arr[4]; int tail; };
struct F { int n; int fam[1]; };
S s;
volatile int vi;
int main() {
  int r = 0;
  vi = 3; r += (a + vi) - a;           // 3: one past, fine
  vi = 0; r += (a + vi) - a;           // 0
  vi = 4; r += (a + vi) - a;           // caught -> 0
  vi = 1; r += (a - vi) - a;           // caught -> 0
  vi = -2; r += (a - vi) - a;          // 2: fine
  vi = 5; r += (vi + a) - a;           // caught -> 0
  unsigned u = -1u;
  r += (a + u) - a;                    // caught -> 0
  vi = 5; r += (s.arr + vi) - s.arr;   // caught -> 0
  vi = 4; r += (m[1] + vi) - m[1];     // caught -> 0
  int *p = a;
  vi = 7; r += (p + vi) - p;           // 7: not checked
  F f;
  F *fp = &f;
  vi = 2; r += (fp->fam + vi) - fp->fam; // 2: flexible, not checked
  std::printf("RESULT r=%d viols=%d\n", r, viols);
  return 0;
}
// CHECK: RESULT r=14 viols=6
