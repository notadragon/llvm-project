// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.invalid.known:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a member function called through pointer arithmetic past the end
// of an array of known bound, `(a + i)->f()`, `((a + i)->*pmf)()`, and a
// member read through it with a pointer to data member, `(a + i)->*pm`, is
// an indirection past the end (ub:expr.unary.dereference.invalid.known), as
// `(a + i)->m` is: caught under "observe", continuing at index 0.  The same
// in range is not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-known-member-call.C.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
struct P {
  int x, y;
  int f() { return y; }
};
P ps[2] = {{1, 2}, {3, 4}};
volatile int vi;
int main() {
  int r = 0;
  int P::*pm = &P::y;
  int (P::*pmf)() = &P::f;
  vi = 1; r += (ps + vi)->f();      // 4: fine
  vi = 2; r += (ps + vi)->f();      // caught -> ps[0].f() = 2
  vi = 2; r += ((ps + vi)->*pmf)(); // caught -> 2
  vi = 2; r += (ps + vi)->*pm;      // caught -> 2
  vi = 1; r += (ps + vi)->*pm;      // 4: fine
  std::printf("RESULT r=%d viols=%d\n", r, viols);
  return 0;
}
// CHECK: RESULT r=14 viols=3
