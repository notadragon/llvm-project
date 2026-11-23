// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all0
// RUN: %t.all0 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind0
// RUN: %t.ind0 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc0
// RUN: %t.acc0 | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.san0
// RUN: %t.san0 | FileCheck %s --check-prefix=SAN
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.san2
// RUN: %t.san2 | FileCheck %s --check-prefix=SAN

// P3100: `(a + i)->*pm` is an indirection through a + i, checked for null
// on a itself (DECISIONS.md E99 (a)) -- also where a + i is converted to
// the base class whose member the pointer to member is first, a null
// pointer conversion the operand of `->*` then is (CLANG-801: the
// conversion hid the arithmetic, and nothing was reported).  A pointer to
// data member, read (.access) or its address taken (.indirection), of a
// polymorphic base at offset zero (`MW`'s primary base `W2`) and at a
// nonzero offset (`MV`'s, after the polymorphic `W1`), and of a base that
// is not polymorphic at a nonzero offset; a call through a pointer to
// member function of a base at a nonzero offset, virtual (.access) or not
// (.indirection; DECISIONS.md E102 (a)).  The controls: the same through
// a itself and without a conversion.  With -fsanitize=null routed too,
// each once.  A fault after the report is caught as SIGSEGV and resumed
// with siglongjmp.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-ptrmem-base-arith*.C;
// gnu_gcc reports each once.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

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
struct W1 {
  virtual void w1() {}
  int q;
};
struct MV : W1, W2 { int z; };
struct S { int a; int b; };
struct Y { long y; };
struct DS : Y, S { int t; };
typedef int W2::*WPD;
typedef void (W2::*WPM)();
NI int mwrd(MW *mp, WPD m) { return (mp + 1)->*m; }
NI int mvrd(MV *mp, WPD m) { return (mp + 1)->*m; }
NI int dsrd(DS *dp, int S::*m) { return (dp + 1)->*m; }
NI int *dsaddr(DS *dp, int S::*m) { return &((dp + 1)->*m); }
NI int *mvaddr(MV *mp, WPD m) { return &((mp + 1)->*m); }
NI void mvpmf(MV *mp, WPM p) { ((mp + 1)->*p)(); }
// The controls.
NI int mwrd0(MW *mp, WPD m) { return mp->*m; }
NI int mvrd0(MV *mp, WPD m) { return mp->*m; }
NI int srd(S *sp, int S::*m) { return (sp + 1)->*m; }

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
  MW *mp = reinterpret_cast<MW *>(nv);
  MV *mvp = reinterpret_cast<MV *>(nv);
  DS *dp = reinterpret_cast<DS *>(nv);
  S *sp = reinterpret_cast<S *>(nv);
  MV mv[2]{};
  DS ds[2]{};
  COUNT("mwrd", mwrd(mp, &W2::r));
  COUNT("mvrd", mvrd(mvp, &W2::r));
  COUNT("dsrd", dsrd(dp, &S::b));
  COUNT("dsaddr", dsaddr(dp, &S::b));
  COUNT("mvaddr", mvaddr(mvp, &W2::r));
  COUNT("mvpmfv", mvpmf(mvp, &W2::w2));
  COUNT("mvpmfnv", mvpmf(mvp, &W2::nw));
  COUNT("mwrd0", mwrd0(mp, &W2::r));
  COUNT("mvrd0", mvrd0(mvp, &W2::r));
  COUNT("srd", srd(sp, &S::b));
  COUNT("nonnull", mvrd(mv, &W2::r); dsrd(ds, &S::b); dsaddr(ds, &S::b);
        mvpmf(mv, &W2::w2); mvpmf(mv, &W2::nw));
  return 0;
}

// ALL: mwrd 1
// ALL-NEXT: mvrd 1
// ALL-NEXT: dsrd 1
// ALL-NEXT: dsaddr 1
// ALL-NEXT: mvaddr 1
// ALL-NEXT: mvpmfv 1
// ALL-NEXT: mvpmfnv 1
// ALL-NEXT: mwrd0 1
// ALL-NEXT: mvrd0 1
// ALL-NEXT: srd 1
// ALL-NEXT: nonnull 0

// IND: mwrd 0
// IND-NEXT: mvrd 0
// IND-NEXT: dsrd 0
// IND-NEXT: dsaddr 1
// IND-NEXT: mvaddr 1
// IND-NEXT: mvpmfv 0
// IND-NEXT: mvpmfnv 1
// IND-NEXT: mwrd0 0
// IND-NEXT: mvrd0 0
// IND-NEXT: srd 0
// IND-NEXT: nonnull 0

// ACC: mwrd 1
// ACC-NEXT: mvrd 1
// ACC-NEXT: dsrd 1
// ACC-NEXT: dsaddr 0
// ACC-NEXT: mvaddr 0
// ACC-NEXT: mvpmfv 1
// ACC-NEXT: mvpmfnv 0
// ACC-NEXT: mwrd0 1
// ACC-NEXT: mvrd0 1
// ACC-NEXT: srd 1
// ACC-NEXT: nonnull 0

// SAN: mwrd 1
// SAN-NEXT: mvrd 1
// SAN-NEXT: dsrd 1
// SAN-NEXT: dsaddr 1
// SAN-NEXT: mvaddr 1
// SAN-NEXT: mvpmfv 1
// SAN-NEXT: mvpmfnv 1
// SAN-NEXT: mwrd0 1
// SAN-NEXT: mvrd0 1
// SAN-NEXT: srd 1
// SAN-NEXT: nonnull 0
