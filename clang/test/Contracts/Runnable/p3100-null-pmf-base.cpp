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

// P3100: a call through a pointer to member function of a base class on
// the object a null pointer points to, converted to the base first --
// `(mp->*p)()`, `((*mp).*p)()` with p a `void (W2::*)()` and W2 a base of
// mp's class, at offset zero (MW's primary base, DW's only base) or not
// (MV's second base), on an element at a constant subscript or offset, on
// a member subobject -- takes the leaf of its P3100 null check from the
// pointer's virtual bit (DECISIONS.md E102 (a)), as a call through a
// pointer to member function of the class itself does: a virtual one,
// which reads the vtable pointer through the object pointer, is reported
// once under .access, a non-virtual one once under .indirection.  A
// non-null pointer is not reported.  (CLANG-800: through `.*` and on
// `mp[1]` the object's conversion to the base checked it under
// .indirection whatever the pointer; CLANG-801: on `mp + 1` the
// conversion hid the arithmetic, and nothing was reported.)  Then Clang's:
// through `.*` on `*(mp + 1)`.  A fault after the report is caught as
// SIGSEGV and resumed with siglongjmp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-pmf-base*.C
// (GCC-864), whose header p3100-null-pmf-base.h has gnu_gcc's forms in
// this order; Clang's is its mwdarith.

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
// W2, polymorphic, is MW's primary base, at offset zero.
struct MW : P, W2 { int z; };
struct W1 {
  virtual void w1() {}
  int q;
};
// W2 is at a nonzero offset, after the polymorphic W1.
struct MV : W1, W2 { int z; };
struct DW : W2 { int z; };
struct HW { int x; MW mw; };
typedef void (W2::*WPM)();
NI void mwpmf(MW *mp, WPM p) { (mp->*p)(); }
NI void mwdpmf(MW *mp, WPM p) { ((*mp).*p)(); }
NI void mvpmf(MV *mp, WPM p) { (mp->*p)(); }
NI void mvdpmf(MV *mp, WPM p) { ((*mp).*p)(); }
NI void dwpmf(DW *dp, WPM p) { (dp->*p)(); }
NI void mwsub(MW *mp, WPM p) { (mp[1].*p)(); }
NI void mvsub(MV *mp, WPM p) { (mp[1].*p)(); }
NI void mwarith(MW *mp, WPM p) { ((mp + 1)->*p)(); }
NI void hwmem(HW *hp, WPM p) { (hp->mw.*p)(); }
NI void hwmemptr(HW *hp, MW HW::*pm, WPM p) { ((hp->*pm).*p)(); }
// Clang's.
NI void mwdarith(MW *mp, WPM p) { ((*(mp + 1)).*p)(); }

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
  DW *dp = reinterpret_cast<DW *>(nv);
  HW *hp = reinterpret_cast<HW *>(nv);
  MW mw[2]{};
  MV mv[2]{};
  HW hw{};
  WPM v = &W2::w2, n = &W2::nw;
  COUNT("mwpmf", mwpmf(mp, n));
  COUNT("mwpmfv", mwpmf(mp, v));
  COUNT("mwdpmf", mwdpmf(mp, n));
  COUNT("mwdpmfv", mwdpmf(mp, v));
  COUNT("mvpmf", mvpmf(mvp, n));
  COUNT("mvpmfv", mvpmf(mvp, v));
  COUNT("mvdpmf", mvdpmf(mvp, n));
  COUNT("mvdpmfv", mvdpmf(mvp, v));
  COUNT("dwpmf", dwpmf(dp, n));
  COUNT("dwpmfv", dwpmf(dp, v));
  COUNT("mwsub", mwsub(mp, n));
  COUNT("mwsubv", mwsub(mp, v));
  COUNT("mvsub", mvsub(mvp, n));
  COUNT("mvsubv", mvsub(mvp, v));
  COUNT("mwarith", mwarith(mp, n));
  COUNT("mwarithv", mwarith(mp, v));
  COUNT("hwmem", hwmem(hp, n));
  COUNT("hwmemv", hwmem(hp, v));
  COUNT("hwmemptr", hwmemptr(hp, &HW::mw, n));
  COUNT("hwmemptrv", hwmemptr(hp, &HW::mw, v));
  COUNT("mwdarith", mwdarith(mp, n));
  COUNT("mwdarithv", mwdarith(mp, v));
  COUNT("nonnull", mwpmf(mw, v); mwdpmf(mw, n); mvpmf(mv, v); mvdpmf(mv, n);
        mwsub(mw, v); mvsub(mv, n); hwmem(&hw, v); mwdarith(mw, v));
  return 0;
}

// ALL: mwpmf 1
// ALL-NEXT: mwpmfv 1
// ALL-NEXT: mwdpmf 1
// ALL-NEXT: mwdpmfv 1
// ALL-NEXT: mvpmf 1
// ALL-NEXT: mvpmfv 1
// ALL-NEXT: mvdpmf 1
// ALL-NEXT: mvdpmfv 1
// ALL-NEXT: dwpmf 1
// ALL-NEXT: dwpmfv 1
// ALL-NEXT: mwsub 1
// ALL-NEXT: mwsubv 1
// ALL-NEXT: mvsub 1
// ALL-NEXT: mvsubv 1
// ALL-NEXT: mwarith 1
// ALL-NEXT: mwarithv 1
// ALL-NEXT: hwmem 1
// ALL-NEXT: hwmemv 1
// ALL-NEXT: hwmemptr 1
// ALL-NEXT: hwmemptrv 1
// ALL-NEXT: mwdarith 1
// ALL-NEXT: mwdarithv 1
// ALL-NEXT: nonnull 0

// IND: mwpmf 1
// IND-NEXT: mwpmfv 0
// IND-NEXT: mwdpmf 1
// IND-NEXT: mwdpmfv 0
// IND-NEXT: mvpmf 1
// IND-NEXT: mvpmfv 0
// IND-NEXT: mvdpmf 1
// IND-NEXT: mvdpmfv 0
// IND-NEXT: dwpmf 1
// IND-NEXT: dwpmfv 0
// IND-NEXT: mwsub 1
// IND-NEXT: mwsubv 0
// IND-NEXT: mvsub 1
// IND-NEXT: mvsubv 0
// IND-NEXT: mwarith 1
// IND-NEXT: mwarithv 0
// IND-NEXT: hwmem 1
// IND-NEXT: hwmemv 0
// IND-NEXT: hwmemptr 1
// IND-NEXT: hwmemptrv 0
// IND-NEXT: mwdarith 1
// IND-NEXT: mwdarithv 0
// IND-NEXT: nonnull 0

// ACC: mwpmf 0
// ACC-NEXT: mwpmfv 1
// ACC-NEXT: mwdpmf 0
// ACC-NEXT: mwdpmfv 1
// ACC-NEXT: mvpmf 0
// ACC-NEXT: mvpmfv 1
// ACC-NEXT: mvdpmf 0
// ACC-NEXT: mvdpmfv 1
// ACC-NEXT: dwpmf 0
// ACC-NEXT: dwpmfv 1
// ACC-NEXT: mwsub 0
// ACC-NEXT: mwsubv 1
// ACC-NEXT: mvsub 0
// ACC-NEXT: mvsubv 1
// ACC-NEXT: mwarith 0
// ACC-NEXT: mwarithv 1
// ACC-NEXT: hwmem 0
// ACC-NEXT: hwmemv 1
// ACC-NEXT: hwmemptr 0
// ACC-NEXT: hwmemptrv 1
// ACC-NEXT: mwdarith 0
// ACC-NEXT: mwdarithv 1
// ACC-NEXT: nonnull 0
