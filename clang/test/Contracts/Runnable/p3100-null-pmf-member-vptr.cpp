// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s --check-prefixes=CHECK,ONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s --check-prefixes=CHECK,ONE
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.n
// RUN: %t.n | FileCheck %s --check-prefixes=CHECK,TWO
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.n2
// RUN: %t.n2 | FileCheck %s --check-prefixes=CHECK,ONE

// P3100: a call through a pointer to member function of a polymorphic
// class on a member subobject of the object a null pointer points to,
// `(hp->m.*p)()`, makes the member access's null check of `hp` after the
// pointer to member function is evaluated (DECISIONS.md E102 (a)); with
// -fsanitize=vptr, the call's vptr check of the subobject, which loads the
// vtable pointer through `hp`, follows that check, so the null dereference
// is reported once before the program faults.  (CLANG-794: the vptr check
// came first and faulted before any report.)  Forms: on a member, an
// element of a member array, the object of `->*`, a member of `*hp`, a
// member of a base at a nonzero offset, an element at a constant
// subscript; virtual and non-virtual pointers to member functions.  A call
// on `*vp` itself, `(vp->*p)()`, whose vptr check tests for null first, is
// the control.  With -fsanitize=null too, each is still reported once
// (DECISIONS.md E111 (a)), but for the call through a pointer to a
// non-virtual member function on `*vp` at -O0, which the sanitizer's check
// of `this` in the member function's prologue reports again (CLANG-792,
// DECISIONS.md E114, open).
// (GCC mirror: g++.dg/ubsan/p3100-null-pmf-member-vptr*.C.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

#define NI __attribute__((noinline))
struct V {
  virtual void h() {}
  void nv() {}
  int v;
};
typedef void (V::*VPM)();
struct H { int x; V m; V arr[2]; };
struct X { long pad; };
struct B { int y; V m; };
struct D : X, B {};
NI void ptr(V *vp, VPM p) { (vp->*p)(); }
NI void mem(H *hp, VPM p) { (hp->m.*p)(); }
NI void memarr(H *hp, VPM p) { (hp->arr[1].*p)(); }
NI void memptr(H *hp, V H::*pm, VPM p) { ((hp->*pm).*p)(); }
NI void dmem(H *hp, VPM p) { ((*hp).m.*p)(); }
NI void basemem(D *dp, VPM p) { (dp->m.*p)(); }
NI void submem(H *hp, VPM p) { (hp[1].m.*p)(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  std::signal(SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  V *vp = reinterpret_cast<V *>(nv);
  H *hp = reinterpret_cast<H *>(nv);
  D *dp = reinterpret_cast<D *>(nv);
  COUNT("ptrv", ptr(vp, &V::h));
  COUNT("ptrnv", ptr(vp, &V::nv));
  COUNT("memv", mem(hp, &V::h));
  COUNT("memnv", mem(hp, &V::nv));
  COUNT("memarrv", memarr(hp, &V::h));
  COUNT("memarrnv", memarr(hp, &V::nv));
  COUNT("memptrv", memptr(hp, &H::m, &V::h));
  COUNT("memptrnv", memptr(hp, &H::m, &V::nv));
  COUNT("dmemv", dmem(hp, &V::h));
  COUNT("dmemnv", dmem(hp, &V::nv));
  COUNT("basememv", basemem(dp, &V::h));
  COUNT("basememnv", basemem(dp, &V::nv));
  COUNT("submemv", submem(hp, &V::h));
  COUNT("submemnv", submem(hp, &V::nv));
  return 0;
}

// CHECK: ptrv 1
// ONE-NEXT: ptrnv 1
// TWO-NEXT: ptrnv 2
// CHECK-NEXT: memv 1
// CHECK-NEXT: memnv 1
// CHECK-NEXT: memarrv 1
// CHECK-NEXT: memarrnv 1
// CHECK-NEXT: memptrv 1
// CHECK-NEXT: memptrnv 1
// CHECK-NEXT: dmemv 1
// CHECK-NEXT: dmemnv 1
// CHECK-NEXT: basememv 1
// CHECK-NEXT: basememnv 1
// CHECK-NEXT: submemv 1
// CHECK-NEXT: submemnv 1
