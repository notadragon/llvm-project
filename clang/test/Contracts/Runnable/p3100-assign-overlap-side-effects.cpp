// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a scalar assignment from an overlapping object
// (ub:expr.assign.overlap) whose operands have side effects is checked, each
// operand evaluated once: the address the value was loaded from is recorded
// by the load itself, so nothing is evaluated twice.  An index incremented in
// the left operand, a volatile source and a source reached through a call are
// caught; the same assignments between disjoint objects are not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-side-effects.C,
// where an operand whose address computation has side effects is evaluated
// through its saved address.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
union U {
  int i;
  short s;
};
union V {
  int i;
  volatile short s;
};
static int calls;
__attribute__((noinline)) U *get(U *p) {
  ++calls;
  return p;
}

__attribute__((noinline)) int incremented(U *p, int k) {
  p[k++].i = p[k].s;
  return k;
}
__attribute__((noinline)) void volatile_src(V *p) { p->i = p->s; }
__attribute__((noinline)) void through_call(U *p, U *q) { p->i = get(q)->s; }

int main() {
  U u[2] = {};
  V v = {};
  through_call(&u[0], &u[1]);
  std::printf("clean=%d calls=%d\n", viols, calls);
  int k = incremented(u, 0);
  volatile_src(&v);
  through_call(&u[0], &u[0]);
  std::printf("caught=%d calls=%d k=%d\n", viols, calls, k);
  return 0;
}
// CHECK: clean=0 calls=1
// CHECK-NEXT: caught=3 calls=2 k=1
