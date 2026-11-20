// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-unused-value -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-unused-value \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// P3100: an indirection through a null pointer whose result is not accessed
// is reported by ub:expr.unary.dereference.nullptr.indirection (CLANG-753,
// DECISIONS.md E86) -- `&*p` (and `&p[0]`, in a template too), a discarded
// `*p;` or `(void)*p`, a static member named through a null pointer
// (`sp->sf()`, `(*sp).sf()`, `sp->k`, `&sp->k`), each of which evaluates
// `*p` ([expr.unary.op], [expr.ref]), and `&*fp` and `*fp;` for a pointer
// to function -- once each under noexcept_observe, at -O0 and -O2, as a
// reference bound to `*p` is; .access alone reports none.  A constexpr
// function's call that is not manifestly constant-evaluated is not folded
// away when the pointer is null; with a pointer to an object, constant
// evaluation is unchanged (the static_asserts).  A non-null pointer reports
// nothing.  More forms: p3100-null-unaccessed-indirection-edges.cpp; folds:
// p3100-null-unaccessed-indirection-fold.cpp.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-unaccessed-indirection.C
// and its -access variant, GCC-799.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

struct S {
  static int k;
  static constexpr int sf() { return 3; }
  int m;
};
int S::k = 4;
using F = int();
int fn() { return 1; }

#define NI __attribute__((noinline))
NI int *addr(int *p) { return &*p; }
NI int *addr0(int *p) { return &p[0]; }
NI void disc(int *p) { *p; }
NI void disc_void(int *p) { (void)*p; }
NI int sfc(S *sp) { return sp->sf(); }
NI int sfc_star(S *sp) { return (*sp).sf(); }
NI int sk(S *sp) { return sp->k; }
NI int *sk_addr(S *sp) { return &sp->k; }
NI int &bind(int *p) { return *p; }
NI F *faddr(F *fp) { return &*fp; }
NI void fdisc(F *fp) { *fp; }
constexpr int sfc_constexpr(S *sp) { return sp->sf(); }
template <class T> NI T *taddr(T *p) { return &*p; }
constexpr int *id(int *p) { return &*p; }
constexpr int g = 0;
constexpr S gs{};
static_assert(id(const_cast<int *>(&g)) == &g);
static_assert(sfc_constexpr(const_cast<S *>(&gs)) == 3);

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports;                                                      \
    __VA_ARGS__;                                                               \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  int *volatile vp = nullptr;
  S *volatile vsp = nullptr;
  F *volatile vfp = nullptr;
  int *p = vp;
  S *sp = vsp;
  F *fp = vfp;
  COUNT("bind", bind(p));
  COUNT("addr", addr(p));
  COUNT("addr0", addr0(p));
  COUNT("disc", disc(p));
  COUNT("disc_void", disc_void(p));
  COUNT("sfc", sfc(sp));
  COUNT("sfc_star", sfc_star(sp));
  COUNT("sk", sk(sp));
  COUNT("sk_addr", sk_addr(sp));
  COUNT("sfc_constexpr", int r = sfc_constexpr(nullptr); (void)r);
  COUNT("taddr", taddr(p));
  COUNT("faddr", faddr(fp));
  COUNT("fdisc", fdisc(fp));
  int x = 0;
  S s{};
  COUNT("nonnull", addr(&x); disc(&x); sfc(&s); sk(&s); taddr(&x);
        faddr(fn); fdisc(fn); int r2 = sfc_constexpr(&s); (void)r2);
  return 0;
}

// IND: bind 1
// IND-NEXT: addr 1
// IND-NEXT: addr0 1
// IND-NEXT: disc 1
// IND-NEXT: disc_void 1
// IND-NEXT: sfc 1
// IND-NEXT: sfc_star 1
// IND-NEXT: sk 1
// IND-NEXT: sk_addr 1
// IND-NEXT: sfc_constexpr 1
// IND-NEXT: taddr 1
// IND-NEXT: faddr 1
// IND-NEXT: fdisc 1
// IND-NEXT: nonnull 0

// ACC: bind 0
// ACC-NEXT: addr 0
// ACC-NEXT: addr0 0
// ACC-NEXT: disc 0
// ACC-NEXT: disc_void 0
// ACC-NEXT: sfc 0
// ACC-NEXT: sfc_star 0
// ACC-NEXT: sk 0
// ACC-NEXT: sk_addr 0
// ACC-NEXT: sfc_constexpr 0
// ACC-NEXT: taddr 0
// ACC-NEXT: faddr 0
// ACC-NEXT: fdisc 0
// ACC-NEXT: nonnull 0
