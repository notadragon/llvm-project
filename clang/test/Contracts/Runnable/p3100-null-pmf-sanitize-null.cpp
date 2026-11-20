// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all0
// RUN: %t.all0 | FileCheck %s --check-prefixes=ALL,ALL0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefixes=ALL,ALL2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind0
// RUN: %t.ind0 | FileCheck %s --check-prefixes=IND,IND0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefixes=IND,IND2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc0
// RUN: %t.acc0 | FileCheck %s --check-prefixes=ACC,ACC0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefixes=ACC,ACC2

// P3100: a call through a pointer to member function of a polymorphic
// class through a null pointer, `(vp->*p)()`, or on a member subobject of
// the object it points to, `(hp->m.*p)()`, takes the leaf of its P3100
// null check from the pointer's virtual bit, so the check is made after
// the pointer to member function is evaluated (DECISIONS.md E102 (a)).
// With -fsanitize=null too, routed to the handler, the contract's check
// replaces the sanitizer's for the pointer it checks, which is made there
// only where the P3100 check is not (its leaf resolves to ignore), so the
// one null dereference is reported once (DECISIONS.md E111 (a); CLANG-790:
// twice), as where a P3100 check and the sanitizer's are at the same place
// (p3100-precedence-null.cpp).  A virtual call on the member and a read of
// it, the controls, are reported once.  Then gnu_gcc's: a non-virtual
// pointer to member function, the object of `->*` and an element at a
// constant subscript as the object, a member of a virtual base; the
// virtual calls -- devirtualized through a `final` class or on the first
// element of a member array (DECISIONS.md E110 (a)), on an element of a
// member array, a primary base's virtual function (offset zero); and a
// pointer to data member applied through a null pointer, at offset zero
// too, and to a member.  These non-pmf forms were each reported once
// before E111 (a) too, and are once under E112 (a).  Then Clang's: on an
// element of a member array, of `*hp`, and on a member of a base at a
// nonzero offset, through `->` and `*`.  Then gnu_gcc's M88 forms: a
// pointer to member function of a base class, at offset zero and not,
// through `->*` and `.*`, on an element at a constant subscript (GCC-864).
//
// Each is reported once in every run, except:
// * at -O0, a call that reaches the member function -- `ptrnv`, `fin`,
//   and the non-virtual calls through a pointer to member function of a
//   base on a null object pointer -- is reported again by the sanitizer's
//   check of `this` in the member function's prologue (CLANG-792,
//   accepted, DECISIONS.md E114 (b)); the optimizer deletes that check at
//   -O2 (`this` is nonnull);
// * on an element at a constant subscript, `(hp[1].m.*p)()` and
//   `(mp[1].*p)()`, where the P3100 check is ignored (`submemv` and
//   `mwsubv` with only .indirection configured, `submemnv` and `mwsubnv`
//   with only .access) the sanitizer checks the element's address, which
//   is not null, and reports nothing; gnu_gcc's checks `hp` (`mp`) and
//   reports it (CLANG-793, accepted, DECISIONS.md E115 (a)).
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-null-pmf-sanitize-null*.C,
// whose header p3100-null-pmf-sanitize-null.h has these forms in this
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
struct V {
  virtual void h() {}
  void nv() {}
  int v;
};
typedef void (V::*VPM)();
struct H { int x; V m; V arr[2]; };
NI void ptr(V *vp, VPM p) { (vp->*p)(); }
NI void mem(H *hp, VPM p) { (hp->m.*p)(); }
NI void call(H *hp) { hp->m.h(); }
NI int rd(H *hp) { return hp->m.v; }
// gnu_gcc's.
struct F final { virtual void h() {} int f; };
struct P { int p; };
struct W2 { virtual void w2() {} void nw() {} int r; };
struct MW : P, W2 { int z; };
struct B { int y; V m; };
struct VD : virtual B { int z; };
struct S { int a; int b; };
struct HS { int x; S s; };
NI void ptrnv(V *vp, VPM p) { (vp->*p)(); }
NI void memptr(H *hp, V H::*pm, VPM p) { ((hp->*pm).*p)(); }
NI void submem(H *hp, VPM p) { (hp[1].m.*p)(); }
NI void vbasemem(VD *vp, VPM p) { (vp->m.*p)(); }
NI void fin(F *fp) { fp->h(); }
NI void decay(H *hp) { hp->arr->h(); }
NI void memarr(H *hp) { hp->arr[1].h(); }
NI void base(MW *mp) { mp->w2(); }
NI int pmd(S *sp, int S::*pm) { return sp->*pm; }
NI int pmdmem(HS *hp, int S::*pm) { return hp->s.*pm; }
// Clang's.
struct X { long pad; };
struct D : X, B {};
NI void memarrp(H *hp, VPM p) { (hp->arr[1].*p)(); }
NI void dmem(H *hp, VPM p) { ((*hp).m.*p)(); }
NI void basemem(D *dp, VPM p) { (dp->m.*p)(); }
NI void dbasemem(D *dp, VPM p) { ((*dp).m.*p)(); }
// gnu_gcc's M88 forms: a pointer to member function of a base class, the
// object converted to the base first: `MW`'s `W2`, its primary base, at
// offset zero, `MV`'s at a nonzero offset, on an element at a constant
// subscript (GCC-864).
struct W1 { virtual void w1() {} int q; };
struct MV : W1, W2 { int z; };
typedef void (W2::*WPM)();
NI void mwpmf(MW *mp, WPM p) { (mp->*p)(); }
NI void mwdpmf(MW *mp, WPM p) { ((*mp).*p)(); }
NI void mvpmf(MV *mp, WPM p) { (mp->*p)(); }
NI void mwsub(MW *mp, WPM p) { (mp[1].*p)(); }

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
  F *fp = reinterpret_cast<F *>(nv);
  MW *mp = reinterpret_cast<MW *>(nv);
  VD *vdp = reinterpret_cast<VD *>(nv);
  S *sp = reinterpret_cast<S *>(nv);
  HS *hsp = reinterpret_cast<HS *>(nv);
  D *dp = reinterpret_cast<D *>(nv);
  MV *mvp = reinterpret_cast<MV *>(nv);
  COUNT("ptrv", ptr(vp, &V::h));
  COUNT("memv", mem(hp, &V::h));
  COUNT("memnv", mem(hp, &V::nv));
  COUNT("call", call(hp));
  COUNT("rd", rd(hp));
  COUNT("ptrnv", ptrnv(vp, &V::nv));
  COUNT("memptrv", memptr(hp, &H::m, &V::h));
  COUNT("memptrnv", memptr(hp, &H::m, &V::nv));
  COUNT("submemv", submem(hp, &V::h));
  COUNT("submemnv", submem(hp, &V::nv));
  COUNT("vbasememv", vbasemem(vdp, &V::h));
  COUNT("vbasememnv", vbasemem(vdp, &V::nv));
  COUNT("fin", fin(fp));
  COUNT("decay", decay(hp));
  COUNT("memarr", memarr(hp));
  COUNT("base", base(mp));
  COUNT("pmd", pmd(sp, &S::b));
  COUNT("pmd0", pmd(sp, &S::a));
  COUNT("pmdmem", pmdmem(hsp, &S::b));
  COUNT("memarrpv", memarrp(hp, &V::h));
  COUNT("memarrpnv", memarrp(hp, &V::nv));
  COUNT("dmemv", dmem(hp, &V::h));
  COUNT("dmemnv", dmem(hp, &V::nv));
  COUNT("basememv", basemem(dp, &V::h));
  COUNT("basememnv", basemem(dp, &V::nv));
  COUNT("dbasememv", dbasemem(dp, &V::h));
  COUNT("dbasememnv", dbasemem(dp, &V::nv));
  COUNT("mwpmfv", mwpmf(mp, &W2::w2));
  COUNT("mwpmfnv", mwpmf(mp, &W2::nw));
  COUNT("mwdpmfv", mwdpmf(mp, &W2::w2));
  COUNT("mwdpmfnv", mwdpmf(mp, &W2::nw));
  COUNT("mvpmfv", mvpmf(mvp, &W2::w2));
  COUNT("mvpmfnv", mvpmf(mvp, &W2::nw));
  COUNT("mwsubv", mwsub(mp, &W2::w2));
  COUNT("mwsubnv", mwsub(mp, &W2::nw));
  return 0;
}

// ALL: ptrv 1
// ALL-NEXT: memv 1
// ALL-NEXT: memnv 1
// ALL-NEXT: call 1
// ALL-NEXT: rd 1
// ALL0-NEXT: ptrnv 2
// ALL2-NEXT: ptrnv 1
// ALL-NEXT: memptrv 1
// ALL-NEXT: memptrnv 1
// ALL-NEXT: submemv 1
// ALL-NEXT: submemnv 1
// ALL-NEXT: vbasememv 1
// ALL-NEXT: vbasememnv 1
// ALL0-NEXT: fin 2
// ALL2-NEXT: fin 1
// ALL-NEXT: decay 1
// ALL-NEXT: memarr 1
// ALL-NEXT: base 1
// ALL-NEXT: pmd 1
// ALL-NEXT: pmd0 1
// ALL-NEXT: pmdmem 1
// ALL-NEXT: memarrpv 1
// ALL-NEXT: memarrpnv 1
// ALL-NEXT: dmemv 1
// ALL-NEXT: dmemnv 1
// ALL-NEXT: basememv 1
// ALL-NEXT: basememnv 1
// ALL-NEXT: dbasememv 1
// ALL-NEXT: dbasememnv 1
// ALL-NEXT: mwpmfv 1
// ALL0-NEXT: mwpmfnv 2
// ALL2-NEXT: mwpmfnv 1
// ALL-NEXT: mwdpmfv 1
// ALL0-NEXT: mwdpmfnv 2
// ALL2-NEXT: mwdpmfnv 1
// ALL-NEXT: mvpmfv 1
// ALL0-NEXT: mvpmfnv 2
// ALL2-NEXT: mvpmfnv 1
// ALL-NEXT: mwsubv 1
// ALL-NEXT: mwsubnv 1

// IND: ptrv 1
// IND-NEXT: memv 1
// IND-NEXT: memnv 1
// IND-NEXT: call 1
// IND-NEXT: rd 1
// IND0-NEXT: ptrnv 2
// IND2-NEXT: ptrnv 1
// IND-NEXT: memptrv 1
// IND-NEXT: memptrnv 1
// IND-NEXT: submemv 0
// IND-NEXT: submemnv 1
// IND-NEXT: vbasememv 1
// IND-NEXT: vbasememnv 1
// IND0-NEXT: fin 2
// IND2-NEXT: fin 1
// IND-NEXT: decay 1
// IND-NEXT: memarr 1
// IND-NEXT: base 1
// IND-NEXT: pmd 1
// IND-NEXT: pmd0 1
// IND-NEXT: pmdmem 1
// IND-NEXT: memarrpv 1
// IND-NEXT: memarrpnv 1
// IND-NEXT: dmemv 1
// IND-NEXT: dmemnv 1
// IND-NEXT: basememv 1
// IND-NEXT: basememnv 1
// IND-NEXT: dbasememv 1
// IND-NEXT: dbasememnv 1
// IND-NEXT: mwpmfv 1
// IND0-NEXT: mwpmfnv 2
// IND2-NEXT: mwpmfnv 1
// IND-NEXT: mwdpmfv 1
// IND0-NEXT: mwdpmfnv 2
// IND2-NEXT: mwdpmfnv 1
// IND-NEXT: mvpmfv 1
// IND0-NEXT: mvpmfnv 2
// IND2-NEXT: mvpmfnv 1
// IND-NEXT: mwsubv 0
// IND-NEXT: mwsubnv 1

// ACC: ptrv 1
// ACC-NEXT: memv 1
// ACC-NEXT: memnv 1
// ACC-NEXT: call 1
// ACC-NEXT: rd 1
// ACC0-NEXT: ptrnv 2
// ACC2-NEXT: ptrnv 1
// ACC-NEXT: memptrv 1
// ACC-NEXT: memptrnv 1
// ACC-NEXT: submemv 1
// ACC-NEXT: submemnv 0
// ACC-NEXT: vbasememv 1
// ACC-NEXT: vbasememnv 1
// ACC0-NEXT: fin 2
// ACC2-NEXT: fin 1
// ACC-NEXT: decay 1
// ACC-NEXT: memarr 1
// ACC-NEXT: base 1
// ACC-NEXT: pmd 1
// ACC-NEXT: pmd0 1
// ACC-NEXT: pmdmem 1
// ACC-NEXT: memarrpv 1
// ACC-NEXT: memarrpnv 1
// ACC-NEXT: dmemv 1
// ACC-NEXT: dmemnv 1
// ACC-NEXT: basememv 1
// ACC-NEXT: basememnv 1
// ACC-NEXT: dbasememv 1
// ACC-NEXT: dbasememnv 1
// ACC-NEXT: mwpmfv 1
// ACC0-NEXT: mwpmfnv 2
// ACC2-NEXT: mwpmfnv 1
// ACC-NEXT: mwdpmfv 1
// ACC0-NEXT: mwdpmfnv 2
// ACC2-NEXT: mwdpmfnv 1
// ACC-NEXT: mvpmfv 1
// ACC0-NEXT: mvpmfnv 2
// ACC2-NEXT: mvpmfnv 1
// ACC-NEXT: mwsubv 1
// ACC-NEXT: mwsubnv 0
