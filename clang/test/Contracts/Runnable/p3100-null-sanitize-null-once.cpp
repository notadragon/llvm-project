// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all0
// RUN: %t.all0 | FileCheck %s --check-prefix=O0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=O2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind0
// RUN: %t.ind0 | FileCheck %s --check-prefix=O0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc0
// RUN: %t.acc0 | FileCheck %s --check-prefix=O0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.ign0
// RUN: %t.ign0 | FileCheck %s --check-prefix=O0

// P3100: with -fsanitize=null routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a member call
// through a null pointer on a subobject at offset zero -- a function of a
// primary base, `mp->W2::w2()`, `mp->nw()`, a member, `hp->m.nv()` -- is
// reported once by the P3100 check at the call, which replaces the
// sanitizer's there (DECISIONS.md E112 (a), E120); a read through a
// downcast of `*vp` to a reference, `static_cast<D &>(*vp).d`, of a
// polymorphic class and of one that is not, twice.  The forms are
// gnu_gcc's (GCC-860), in the order of its header.
//
// Each is reported once, except:
// * at -O0 each call is reported again by the sanitizer's check of `this`
//   in the member function's prologue (CLANG-792, accepted, DECISIONS.md
//   E114 (b)); the optimizer deletes it at -O2 (`this` is nonnull);
// * a read through a downcast is reported by the P3100 check of the read
//   and by the sanitizer's null check of the downcast
//   (`TCK_DowncastReference`), and with the group ignored by the
//   sanitizer's two checks, as stock -fsanitize=null alone: twice
//   everywhere (CLANG-796, accepted under DECISIONS.md E120: dropping the
//   downcast's check only where the read's P3100 check is made needs the
//   read's leaf passed to the downcast; gnu_gcc reports it once where a
//   null leaf applies and twice with the group ignored).
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-null-sanitize-null-once*.C,
// whose header p3100-null-sanitize-null-once.h has these forms in this
// order.

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
struct P { int p; };
struct W2 {
  virtual void w2() {}
  void nw() {}
  int r;
};
struct MW : P, W2 { int z; };
struct V {
  virtual void h() {}
  void nv() {}
  int v;
};
struct D : V { int d; };
struct H0 { V m; int x; };
struct S { int s; };
struct DS : S { int t; };
NI void mwq(MW *mp) { mp->W2::w2(); }
NI void mwnw(MW *mp) { mp->nw(); }
NI void h0nv(H0 *hp) { hp->m.nv(); }
NI int drdc(V *vp) { return static_cast<D &>(*vp).d; }
NI int dsrd(S *sp) { return static_cast<DS &>(*sp).t; }

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
  COUNT("mwq", mwq(reinterpret_cast<MW *>(nv)));
  COUNT("mwnw", mwnw(reinterpret_cast<MW *>(nv)));
  COUNT("h0nv", h0nv(reinterpret_cast<H0 *>(nv)));
  COUNT("drdc", drdc(reinterpret_cast<V *>(nv)));
  COUNT("dsrd", dsrd(reinterpret_cast<S *>(nv)));
  return 0;
}

// O0: mwq 2
// O0-NEXT: mwnw 2
// O0-NEXT: h0nv 2
// O0-NEXT: drdc 2
// O0-NEXT: dsrd 2

// O2: mwq 1
// O2-NEXT: mwnw 1
// O2-NEXT: h0nv 1
// O2-NEXT: drdc 2
// O2-NEXT: dsrd 2
