// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o0
// RUN: %t.o0 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O1 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o1
// RUN: %t.o1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.o2
// RUN: %t.o2 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O1 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.s1
// RUN: %t.s1 | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.s2
// RUN: %t.s2 | FileCheck %s

// P3100: the caller of a function that is not inlined and whose only call
// is a member call through a null pointer of a member function that does
// nothing sees the effects of the contract-violation handler the
// function's null check calls: `reports`, which the handler alone
// changes, is read again after the call, at -O0, -O1 and -O2.  The same
// with -fsanitize=null routed to the handler and the P3100 check ignored
// (so the sanitizer's check is made), at -O1 and -O2 (at -O0 the member
// function's prologue reports it again, CLANG-792).  The forms: a
// non-virtual member function returning a constant, a non-virtual one of a
// polymorphic class, a call devirtualized through a `final` class, and,
// the control, a call through a pointer to a non-virtual member function.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-handler-effects-ipa*.C and
// g++.dg/ubsan/p3100-routed-handler-effects-ipa.C, GCC-857, where
// ipa-reference took such a function to touch no static.  -flto is not run
// here.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

#define NI __attribute__((noinline))
struct S {
  NI int g() { return 0; }
};
struct V {
  virtual void h() {}
  NI void nv() {}
};
typedef void (V::*VPM)();
struct F final {
  virtual void h() {}
};
NI void plain(S *sp) { sp->g(); }
NI void poly(V *vp) { vp->nv(); }
NI void fin(F *fp) { fp->h(); }
NI void ptrnv(V *vp, VPM p) { (vp->*p)(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int *volatile nv = nullptr;
  COUNT("plain", plain(reinterpret_cast<S *>(nv)));
  COUNT("poly", poly(reinterpret_cast<V *>(nv)));
  COUNT("fin", fin(reinterpret_cast<F *>(nv)));
  COUNT("ptrnv", ptrnv(reinterpret_cast<V *>(nv), &V::nv));
  return 0;
}

// CHECK: plain 1
// CHECK-NEXT: poly 1
// CHECK-NEXT: fin 1
// CHECK-NEXT: ptrnv 1
