// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND

// P3100: with -fsanitize=vptr routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured, a member call or
// access through a null pointer to an object of a polymorphic class is
// reported as without -fsanitize=vptr: once, under the leaf of the form
// (DECISIONS.md E94 (b), E102 (a), E110 (a)).  The vptr check of a member
// call or access, of a downcast to a reference or of a conversion to a
// virtual base tests the object pointer for null before it reads the
// vtable pointer (stock), so it reports nothing more.  A fault after the
// report is caught as SIGSEGV and resumed with siglongjmp, so one run
// counts the reports of every form.  The forms: a virtual and a
// non-virtual call, a read and a write; a read of and a call on a member;
// a qualified call; a virtual call and a read through a base at offset
// zero; a call devirtualized through a `final` class; a virtual call on
// an element of a member array; a virtual call, a read and a qualified
// call of a polymorphic base's function or member -- `MW`'s `W2`, which,
// `P` not being polymorphic, is its primary base, at offset zero; a read,
// a virtual and a non-virtual call through a virtual base; a call on
// `*vp`, on `vp[1]`, and a reference bound to `*vp`; a downcast of `*vp`
// to a reference and a read through it; a reference to the virtual base
// bound to `*vbp` and a virtual call on `vbp[1]`.  Then the same three
// forms of a base at a nonzero offset, `MV`'s `W2` after the polymorphic
// `W1`; and a read of `vbp[1]`'s member of the virtual base, which loads
// the base's offset through the element (.access; gnu_gcc's GCC-861).
// Mirror: gcc/testsuite/g++.dg/ubsan/p3100-null-vptr-call*.C (GCC-856),
// whose header p3100-null-vptr-call.h has these forms in this order.

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
struct H { int x; V m; V arr[2]; };
struct D : V { int d; };
struct F final {
  virtual void h() {}
  int f;
};
struct P { int p; };
struct W2 {
  virtual void w2() {}
  int r;
};
struct MW : P, W2 { int z; };
struct W1 {
  virtual void w1() {}
  int q;
};
struct MV : W1, W2 { int z; };
struct VB : virtual V { int w; };
NI void vh(V *vp) { vp->h(); }
NI void vnv(V *vp) { vp->nv(); }
NI int vrd(V *vp) { return vp->v; }
NI void vwr(V *vp) { vp->v = 1; }
NI int hrd(H *hp) { return hp->m.v; }
NI void hnv(H *hp) { hp->m.nv(); }
NI void qh(V *vp) { vp->V::h(); }
NI void dh(D *dp) { dp->h(); }
NI int drd(D *dp) { return dp->v; }
NI void fin(F *fp) { fp->h(); }
NI void memarr(H *hp) { hp->arr[1].h(); }
NI void base(MW *mp) { mp->w2(); }
NI int mwrd(MW *mp) { return mp->r; }
NI void mwq(MW *mp) { mp->W2::w2(); }
NI int vbrd(VB *p) { return p->v; }
NI void vbh(VB *p) { p->h(); }
NI void vbnv(VB *p) { p->nv(); }
NI void star(V *vp) { (*vp).nv(); }
NI void sub(V *vp) { vp[1].nv(); }
NI V &bind(V *vp) { return *vp; }
NI D &dref(V *vp) { return static_cast<D &>(*vp); }
NI int drdc(V *vp) { return static_cast<D &>(*vp).d; }
NI V &vbref(VB *p) { return *p; }
NI void vbsub(VB *p) { p[1].h(); }
NI void mvh(MV *mp) { mp->w2(); }
NI int mvrd(MV *mp) { return mp->r; }
NI void mvq(MV *mp) { mp->W2::w2(); }
NI int vbsubrd(VB *p) { return p[1].v; }

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
  F *fp = reinterpret_cast<F *>(nv);
  MW *mp = reinterpret_cast<MW *>(nv);
  VB *vbp = reinterpret_cast<VB *>(nv);
  MV *mvp = reinterpret_cast<MV *>(nv);
  COUNT("vh", vh(vp));
  COUNT("vnv", vnv(vp));
  COUNT("vrd", vrd(vp));
  COUNT("vwr", vwr(vp));
  COUNT("hrd", hrd(hp));
  COUNT("hnv", hnv(hp));
  COUNT("qh", qh(vp));
  COUNT("dh", dh(dp));
  COUNT("drd", drd(dp));
  COUNT("fin", fin(fp));
  COUNT("memarr", memarr(hp));
  COUNT("base", base(mp));
  COUNT("mwrd", mwrd(mp));
  COUNT("mwq", mwq(mp));
  COUNT("vbrd", vbrd(vbp));
  COUNT("vbh", vbh(vbp));
  COUNT("vbnv", vbnv(vbp));
  COUNT("star", star(vp));
  COUNT("sub", sub(vp));
  COUNT("bind", bind(vp));
  COUNT("dref", dref(vp));
  COUNT("drdc", drdc(vp));
  COUNT("vbref", vbref(vbp));
  COUNT("vbsub", vbsub(vbp));
  COUNT("mvh", mvh(mvp));
  COUNT("mvrd", mvrd(mvp));
  COUNT("mvq", mvq(mvp));
  COUNT("vbsubrd", vbsubrd(vbp));
  return 0;
}

// ALL: vh 1
// ALL-NEXT: vnv 1
// ALL-NEXT: vrd 1
// ALL-NEXT: vwr 1
// ALL-NEXT: hrd 1
// ALL-NEXT: hnv 1
// ALL-NEXT: qh 1
// ALL-NEXT: dh 1
// ALL-NEXT: drd 1
// ALL-NEXT: fin 1
// ALL-NEXT: memarr 1
// ALL-NEXT: base 1
// ALL-NEXT: mwrd 1
// ALL-NEXT: mwq 1
// ALL-NEXT: vbrd 1
// ALL-NEXT: vbh 1
// ALL-NEXT: vbnv 1
// ALL-NEXT: star 1
// ALL-NEXT: sub 1
// ALL-NEXT: bind 1
// ALL-NEXT: dref 1
// ALL-NEXT: drdc 1
// ALL-NEXT: vbref 1
// ALL-NEXT: vbsub 1
// ALL-NEXT: mvh 1
// ALL-NEXT: mvrd 1
// ALL-NEXT: mvq 1
// ALL-NEXT: vbsubrd 1
// ACC: vh 1
// ACC-NEXT: vnv 0
// ACC-NEXT: vrd 1
// ACC-NEXT: vwr 1
// ACC-NEXT: hrd 1
// ACC-NEXT: hnv 0
// ACC-NEXT: qh 0
// ACC-NEXT: dh 1
// ACC-NEXT: drd 1
// ACC-NEXT: fin 1
// ACC-NEXT: memarr 1
// ACC-NEXT: base 1
// ACC-NEXT: mwrd 1
// ACC-NEXT: mwq 0
// ACC-NEXT: vbrd 1
// ACC-NEXT: vbh 1
// ACC-NEXT: vbnv 1
// ACC-NEXT: star 0
// ACC-NEXT: sub 0
// ACC-NEXT: bind 0
// ACC-NEXT: dref 0
// ACC-NEXT: drdc 1
// ACC-NEXT: vbref 1
// ACC-NEXT: vbsub 1
// ACC-NEXT: mvh 1
// ACC-NEXT: mvrd 1
// ACC-NEXT: mvq 0
// ACC-NEXT: vbsubrd 1
// IND: vh 0
// IND-NEXT: vnv 1
// IND-NEXT: vrd 0
// IND-NEXT: vwr 0
// IND-NEXT: hrd 0
// IND-NEXT: hnv 1
// IND-NEXT: qh 1
// IND-NEXT: dh 0
// IND-NEXT: drd 0
// IND-NEXT: fin 0
// IND-NEXT: memarr 0
// IND-NEXT: base 0
// IND-NEXT: mwrd 0
// IND-NEXT: mwq 1
// IND-NEXT: vbrd 0
// IND-NEXT: vbh 0
// IND-NEXT: vbnv 0
// IND-NEXT: star 1
// IND-NEXT: sub 1
// IND-NEXT: bind 1
// IND-NEXT: dref 1
// IND-NEXT: drdc 0
// IND-NEXT: vbref 0
// IND-NEXT: vbsub 0
// IND-NEXT: mvh 0
// IND-NEXT: mvrd 0
// IND-NEXT: mvq 1
// IND-NEXT: vbsubrd 0
