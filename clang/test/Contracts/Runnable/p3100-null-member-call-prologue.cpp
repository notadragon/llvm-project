// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s --check-prefix=O2

// P3100: a call to a member function that is not inlined, through a null
// pointer, with -fsanitize=null routed to the handler as well as the P3100
// null check: the P3100 check at the call replaces the sanitizer's there,
// and the sanitizer's check of `this` in the member function's prologue
// (stock Clang's, which -fsanitize=null alone makes as well as the one at
// the call) reports the same null dereference again at -O0; at -O2 the
// optimizer deletes the prologue check, `this` being nonnull: once.
// CLANG-792, accepted (DECISIONS.md E114 (b), by the general rule E120):
// the prologue check is the only check for a caller compiled without the
// P3100 check or with its leaf ignored.  Forms: a non-virtual member
// function of a class that is not polymorphic, and of one that is, a call
// through a pointer to a non-virtual member function, and a call
// devirtualized through a `final` class.
// (GCC: gnu_gcc makes no prologue check and reports each once.)

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
NI void ptrnv(V *vp, VPM p) { (vp->*p)(); }
NI void fin(F *fp) { fp->h(); }

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
  COUNT("ptrnv", ptrnv(reinterpret_cast<V *>(nv), &V::nv));
  COUNT("fin", fin(reinterpret_cast<F *>(nv)));
  return 0;
}

// CHECK: plain 2
// CHECK-NEXT: poly 2
// CHECK-NEXT: ptrnv 2
// CHECK-NEXT: fin 2

// O2: plain 1
// O2-NEXT: poly 1
// O2-NEXT: ptrnv 1
// O2-NEXT: fin 1
