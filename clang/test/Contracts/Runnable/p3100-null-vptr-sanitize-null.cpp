// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all0
// RUN: %t.all0 | FileCheck %s --check-prefixes=ALL,ALL0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefixes=ALL,ALL2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind0
// RUN: %t.ind0 | FileCheck %s --check-prefixes=ALL,ALL0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefixes=ALL,ALL2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc0
// RUN: %t.acc0 | FileCheck %s --check-prefixes=ACC,ACC0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe,vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefixes=ACC,ACC2

// P3100: with -fsanitize=null and -fsanitize=vptr routed to the
// contract-violation handler as well as ub:expr.unary.dereference.nullptr
// configured, a non-virtual member call through a null pointer is reported
// as with -fsanitize=null alone.  Each run prints the number of reports of
// one form, and ` fault` if the program faulted; none of the called
// functions reads through its object pointer, so a fault is the vptr
// check's, reading the vtable pointer through the address it checks.  It
// is caught as SIGSEGV and resumed with siglongjmp.  The forms: a
// qualified virtual and a non-virtual call of a polymorphic base at a
// nonzero offset, `MV`'s `W2` after the polymorphic `W1`; a non-virtual
// call on a member at a nonzero offset; a non-virtual call on `vp[1]` and
// on `*vp`.
//
// Each is reported once, except:
// * the vptr check of the first four (stock) reads through the
//   subobject's address, which is not null, and faults, after the P3100
//   report -- CLANG-793, accepted (DECISIONS.md E115 (a)); gnu_gcc's vptr
//   guard tests the null pointer and does not fault (GCC-856, GCC-865,
//   DECISIONS.md E116 (a));
// * with only .access configured, `vp[1].nv()` is not reported: the
//   sanitizer, which makes the null check of the .indirection leaf there,
//   tests `vp + 1` (CLANG-793; gnu_gcc's GCC-860, DECISIONS.md E121 (a));
// * at -O0, `vp->nv()` is reported again by the sanitizer's check of
//   `this` in the member function's prologue (CLANG-792, accepted,
//   DECISIONS.md E114 (b)); the optimizer deletes it at -O2.
// The .indirection runs match the whole group's.
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-null-vptr-sanitize-null*.C
// (GCC-865), whose header p3100-null-vptr-sanitize-null.h has these forms
// in this order.

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
struct H { int x; V m; V arr[2]; };
struct W1 {
  virtual void w1() {}
  int q;
};
struct W2 {
  virtual void w2() {}
  void nw() {}
  int r;
};
struct MV : W1, W2 { int z; };
NI void mvq(MV *mp) { mp->W2::w2(); }
NI void mvnw(MV *mp) { mp->nw(); }
NI void hnv(H *hp) { hp->m.nv(); }
NI void sub(V *vp) { vp[1].nv(); }
NI void vnv(V *vp) { vp->nv(); }

#define COUNT(name, ...)                                                       \
  do {                                                                         \
    int before = reports, fbefore = faults;                                    \
    if (!sigsetjmp(env, 1)) {                                                  \
      __VA_ARGS__;                                                             \
    }                                                                          \
    std::printf(name " %d%s\n", reports - before,                              \
                faults != fbefore ? " fault" : "");                            \
  } while (0)

int main() {
  std::signal(SIGSEGV, on_segv);
  int *volatile nv = nullptr;
  V *vp = reinterpret_cast<V *>(nv);
  H *hp = reinterpret_cast<H *>(nv);
  MV *mvp = reinterpret_cast<MV *>(nv);
  COUNT("mvq", mvq(mvp));
  COUNT("mvnw", mvnw(mvp));
  COUNT("hnv", hnv(hp));
  COUNT("sub", sub(vp));
  COUNT("vnv", vnv(vp));
  return 0;
}

// ALL: mvq 1 fault
// ALL-NEXT: mvnw 1 fault
// ALL-NEXT: hnv 1 fault
// ALL-NEXT: sub 1 fault
// ALL0-NEXT: vnv 2{{$}}
// ALL2-NEXT: vnv 1{{$}}

// ACC: mvq 1 fault
// ACC-NEXT: mvnw 1 fault
// ACC-NEXT: hnv 1 fault
// ACC-NEXT: sub 0 fault
// ACC0-NEXT: vnv 2{{$}}
// ACC2-NEXT: vnv 1{{$}}
