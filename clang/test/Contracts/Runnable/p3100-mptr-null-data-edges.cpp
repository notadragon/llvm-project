// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a null pointer to data member
// (ub:expr.mptr.oper.member.func.null.data) under "observe": `.*` and `->*`
// with a null pointer are caught, through a base too; a pointer to the first
// member (offset 0) and any other non-null pointer are not, nor is a
// constant pointer, nor an unevaluated operand.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-data-edges.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
}
struct S {
  int a;
  int m;
};
struct B {
  long pad;
};
struct D : B, S {};
int S::*volatile pnull = nullptr;
int S::*volatile pa = &S::a;
int S::*volatile pm = &S::m;
__attribute__((noinline)) int *dot(S &s, int S::*p) { return &(s.*p); }
__attribute__((noinline)) int *arrow(S *s, int S::*p) { return &(s->*p); }
__attribute__((noinline)) int *based(D &d, int S::*p) { return &(d.*p); }
__attribute__((noinline)) int *konst(S &s) { return &(s.*(&S::m)); }
int main() {
  S s{};
  D d{};
  int r = 0;
  r += dot(s, pa) == &s.a; // offset 0: not null
  r += dot(s, pm) == &s.m;
  r += arrow(&s, pm) == &s.m;
  r += based(d, pm) == &d.m;
  r += konst(s) == &s.m;
  r += sizeof(s.*pnull) == sizeof(int); // unevaluated
  std::printf("OK=%d viols=%d\n", r, viols);
  dot(s, pnull);
  arrow(&s, pnull);
  based(d, pnull);
  std::printf("viols=%d\n", viols);
  return 0;
}
// CHECK: OK=6 viols=0
// CHECK-NEXT: viols=3
