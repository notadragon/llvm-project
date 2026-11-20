// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: an indirection past the end of an array of known bound
// (ub:expr.unary.dereference.invalid.known) under "observe".  &a[i] with i the
// element count, and *(a + i) or (a + i)->m reading past the end, are caught
// and continue at index 0, also (a + i)->m with m a member of a base class
// at a nonzero offset (CLANG-747); in-range indirections, the one-past
// pointer a + N formed without an indirection, and an access a[N] (the
// subscript check's) are not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-known-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
struct P { int x, y; };
int a[3] = {10, 20, 30};
int b[4] = {10, 20, 30, 40};
P ps[2] = {{1, 2}, {3, 4}};
struct Q0 { int z; };
struct Q : Q0, P {};
Q qs[2] = {{{9}, {1, 2}}, {{9}, {3, 4}}};
volatile int vi;
int main() {
  int r = 0;
  vi = 2; r += &a[vi] - a;    // 2: fine
  vi = 3; r += &a[vi] - a;    // caught -> 0
  vi = 3; r += *(b + vi);     // 40: fine
  vi = 3; r += *(a + vi);     // caught -> a[0] = 10
  vi = 2; r += (ps + vi)->y;  // caught -> ps[0].y = 2
  vi = 2; r += (qs + vi)->y;  // caught -> qs[0].y = 2
  vi = 3; r += (a + vi) - a;  // 3: no indirection
  vi = 3; r += b[vi];         // 40: an access, not this check
  std::printf("RESULT r=%d viols=%d\n", r, viols);
  return 0;
}
// CHECK: RESULT r=99 viols=4
