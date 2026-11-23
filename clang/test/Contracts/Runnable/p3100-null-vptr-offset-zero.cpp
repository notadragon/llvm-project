// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.i
// RUN: %t.i | FileCheck %s --check-prefix=NONE

// P3100: with -fsanitize=vptr routed to the contract-violation handler and
// -fcontracts-p3100, the vptr check of a member call or member access on
// an object at offset zero from a null pointer -- a function or member of
// a primary base (`mp->W2::w2()`, `mp->nw()`, `&mp->r`, `((*mp).*p)()`) or
// of a member subobject at offset zero (`hp->m.nv()`, `&hp->m.v`) -- tests
// the pointer for null before it reads the vtable pointer, as without
// -fcontracts-p3100: the program does not fault, whether the P3100 check
// of the pointer, made by the conversion to the base or the member access,
// reports and continues or is ignored.  (CLANG-797: the type check skips
// its null check because that P3100 check was made elsewhere, and the vptr
// check took the skip for a non-null guarantee and faulted.)  A call
// through `->*` on the base is the control.
// (GCC mirror: g++.dg/ubsan/p3100-null-vptr-offset-zero*.C.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports, faults;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

static sigjmp_buf env;
static void on_segv(int) {
  ++faults;
  siglongjmp(env, 1);
}

#define NI __attribute__((noinline))
struct V {
  virtual void h() {}
  void nv() {}
  int v;
};
struct P { int p; };
struct W2 {
  virtual void w2() {}
  void nw() {}
  int r;
};
struct MW : P, W2 { int z; }; // W2, polymorphic, is the primary base.
struct H0 { V m; int x; };
typedef void (W2::*WPM)();
NI void mwq(MW *mp) { mp->W2::w2(); }
NI void mwnw(MW *mp) { mp->nw(); }
NI int *mwaddr(MW *mp) { return &mp->r; }
NI void mwdpmf(MW *mp, WPM p) { ((*mp).*p)(); }
NI void h0nv(H0 *hp) { hp->m.nv(); }
NI int *h0addr(H0 *hp) { return &hp->m.v; }
NI void mwpmf(MW *mp, WPM p) { (mp->*p)(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports, fb = faults;                                         \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d%s\n", reports - before,                              \
                faults != fb ? " fault" : "");                                 \
  } while (0)

int main() {
  std::signal(SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  MW *mp = reinterpret_cast<MW *>(nv);
  H0 *hp = reinterpret_cast<H0 *>(nv);
  COUNT("mwq", mwq(mp));
  COUNT("mwnw", mwnw(mp));
  COUNT("mwaddr", mwaddr(mp));
  COUNT("mwdpmf", mwdpmf(mp, &W2::nw));
  COUNT("h0nv", h0nv(hp));
  COUNT("h0addr", h0addr(hp));
  COUNT("mwpmf", mwpmf(mp, &W2::nw));
  return 0;
}

// ALL: mwq 1{{$}}
// ALL-NEXT: mwnw 1{{$}}
// ALL-NEXT: mwaddr 1{{$}}
// ALL-NEXT: mwdpmf 1{{$}}
// ALL-NEXT: h0nv 1{{$}}
// ALL-NEXT: h0addr 1{{$}}
// ALL-NEXT: mwpmf 1{{$}}
// NONE: mwq 0{{$}}
// NONE-NEXT: mwnw 0{{$}}
// NONE-NEXT: mwaddr 0{{$}}
// NONE-NEXT: mwdpmf 0{{$}}
// NONE-NEXT: h0nv 0{{$}}
// NONE-NEXT: h0addr 0{{$}}
// NONE-NEXT: mwpmf 0{{$}}
