// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.assign.overlap:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a scalar assignment from an overlapping object
// (ub:expr.assign.overlap) under "observe": an inexact overlap, or an exact
// one between objects of different types, is caught -- union members by name
// or through a pointer, through a conversion of the value read, and between
// misaligned objects of the same type; an exact overlap of the same type, two
// different variables, disjoint members, a value computed rather than read,
// a compound assignment and bit-fields are not.  An assignment whose value is
// used is checked too.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assign-overlap-edges.C)

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
  unsigned u;
  float f;
};
struct S {
  int a;
  int b;
  int c : 4;
  int d : 4;
};
typedef short short1 __attribute__((aligned(1)));

__attribute__((noinline)) void by_name() {
  U u;
  u.s = 1;
  u.i = u.s;
}
__attribute__((noinline)) void by_pointer(U *p) { p->i = p->s; }
__attribute__((noinline)) void exact_diff_type(U *p) { p->i = p->u; }
__attribute__((noinline)) void converted(U *p) { p->i = (int)p->s; }
__attribute__((noinline)) void floating(U *p) { p->f = p->i; }
__attribute__((noinline)) void misaligned(char *buf) {
  *(short1 *)buf = *(short1 *)(buf + 1);
}
__attribute__((noinline)) int value_used(U *p) { return (p->i = p->s); }

__attribute__((noinline)) void same(int *p, int *q) { *p = *q; }
__attribute__((noinline)) void self(U *p) { p->i = p->i; }
__attribute__((noinline)) void vars(int &x, int y) {
  x = y;
  int z = 0;
  z = x;
  x = z;
}
__attribute__((noinline)) void members(S *p) { p->a = p->b; }
__attribute__((noinline)) void computed(U *p) { p->i = p->s + 1; }
__attribute__((noinline)) void compound(U *p) { p->i += p->s; }
__attribute__((noinline)) void bitfields(S *p) { p->c = p->d; }

int main() {
  U u = {0};
  S s = {1, 2, 3, 4};
  int x = 5;
  alignas(4) char buf[8] = {0};
  same(&x, &x);
  self(&u);
  vars(x, 6);
  members(&s);
  computed(&u);
  compound(&u);
  bitfields(&s);
  std::printf("clean=%d\n", viols);
  by_name();
  by_pointer(&u);
  exact_diff_type(&u);
  converted(&u);
  floating(&u);
  misaligned(buf);
  value_used(&u);
  std::printf("caught=%d\n", viols);
  return 0;
}
// CHECK: clean=0
// CHECK-NEXT: caught=7
