// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a call through a null pointer to member function
// (ub:expr.mptr.oper.member.func.null.function) under "observe": a null
// pointer is caught with `.*` and `->*`, for a polymorphic class and a
// non-polymorphic one; pointers to a non-virtual function, to the first
// virtual function and to a later one are not.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-fn-edges.C)

#include <contracts>
#include <csetjmp>
#include <cstdio>
namespace cs = std::contracts;
static std::jmp_buf env;
static int viols;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++viols;
  std::longjmp(env, 1); // do not call through the null pointer
}
struct S {
  int f() { return 1; }
  virtual int g() { return 2; }
  virtual int h() { return 3; }
};
struct N {
  int f() { return 4; }
};
typedef int (S::*PS)();
typedef int (N::*PN)();
PS volatile snull = nullptr;
PN volatile nnull = nullptr;
__attribute__((noinline)) int dot(S &s, PS p) { return (s.*p)(); }
__attribute__((noinline)) int arrow(S *s, PS p) { return (s->*p)(); }
__attribute__((noinline)) int ndot(PN p) {
  N n;
  return (n.*p)();
}
int main() {
  S s;
  int r = dot(s, &S::f) + dot(s, &S::g) + arrow(&s, &S::h) + ndot(&N::f);
  std::printf("SUM=%d viols=%d\n", r, viols);
  if (setjmp(env) == 0)
    dot(s, snull);
  if (setjmp(env) == 0)
    arrow(&s, snull);
  if (setjmp(env) == 0)
    ndot(nnull);
  std::printf("viols=%d\n", viols);
  return 0;
}
// CHECK: SUM=10 viols=0
// CHECK-NEXT: viols=3
