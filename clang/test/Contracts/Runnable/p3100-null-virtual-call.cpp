// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all
// RUN: %t.all | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2
// RUN: %t.all2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC

// P3100: a virtual call through a null pointer reads the vtable pointer
// through it, so it is reported once under .access (DECISIONS.md E102
// (a)): `vp->h()`, `(*vp).h()`, on an element `vp[1]` or `vp[i]` and at
// an offset `vp + 1` or `vp + i` (which check `vp`), a virtual function of
// a polymorphic base at a nonzero offset, on an element of a member array
// `hp->arr[1]` and its first element through `hp->arr->`, and a virtual
// destructor called explicitly.  The leaf follows the source: so is a
// call to a virtual function that is devirtualized, on a member subobject
// `hp->m` (whose dynamic type is known), through a `final` class or a
// `final` overrider (DECISIONS.md E110 (a)).  A qualified call and a
// non-virtual one are reported once under .indirection.  A non-null
// pointer is not reported.
// (CLANG-781: every member function call was .indirection; the
// devirtualized calls were until E110 (a).)  The
// dynamic_cast to a reference is in
// Runnable/p3100-null-dynamic-cast-reference.cpp, a call through a pointer
// to member function in Runnable/p3100-null-virtual-pmf.cpp.  Then
// gnu_gcc's: a base's virtual function at a nonzero offset through `*mp`,
// on `mp[1]` and `mp + 1`, and one on an element of a member array of a
// member, each once under .access; one called through `this` in a member
// function or a lambda called through a null pointer (DECISIONS.md E106
// (a)) or through a reference bound to `*mp`: only the call or the binding
// is reported, once under .indirection.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-virtual-call*.C,
// whose header p3100-null-virtual-call.h has the same forms in this order.
// gnu_gcc does not check a non-constant offset on `vp` (the CLANG-745
// divergence); it reports the devirtualized calls under .access too.

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
struct V {
  virtual void h() {}
  void nv() {}
  int v;
};
struct P { int p; };
struct W2 { virtual void w2() {} int r; };
struct MW : P, W2 {
  int z;
  NI void g() { w2(); }
  NI void gl() { [this] { w2(); }(); }
};
struct H { int x; V m; V arr[2]; };
struct HH { int y; H h; };
struct VD { virtual ~VD() {} int d; };
struct F final { virtual void h() {} int f; };
struct D : V { void h() final {} };
NI void vcall(V *vp) { vp->h(); }
NI void vdcall(V *vp) { (*vp).h(); }
NI void vsub(V *vp) { vp[1].h(); }
NI void vsubi(V *vp, int i) { vp[i].h(); }
NI void varith(V *vp) { (vp + 1)->h(); }
NI void varithi(V *vp, int i) { (vp + i)->h(); }
NI void vbase(MW *mp) { mp->w2(); }
NI void vmemarr(H *hp) { hp->arr[1].h(); }
NI void vmemdecay(H *hp) { hp->arr->h(); }
NI void vdtor(VD *dp) { dp->~VD(); }
NI void vmember(H *hp) { hp->m.h(); }
NI void final(F *fp) { fp->h(); }
NI void qual(V *vp) { vp->V::h(); }
NI void dfinal(D *dp) { dp->h(); }
NI void nonvirt(V *vp) { vp->nv(); }
// gnu_gcc's: a base's virtual function at a nonzero offset through `*mp`,
// an element at a constant subscript or offset, `this` (DECISIONS.md E106
// (a)) or a reference, and one on an element of a member array of a
// member.
NI void vdbase(MW *mp) { (*mp).w2(); }
NI void vsubbase(MW *mp) { mp[1].w2(); }
NI void varithbase(MW *mp) { (mp + 1)->w2(); }
NI void vthis(MW *mp) { mp->g(); }
NI void vlambda(MW *mp) { mp->gl(); }
NI void vrefbody(MW &r) { r.w2(); }
NI void vref(MW *mp) { vrefbody(*mp); }
NI void vnested(HH *hp) { hp->h.arr[1].h(); }

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
  MW *mp = reinterpret_cast<MW *>(nv);
  H *hp = reinterpret_cast<H *>(nv);
  VD *vdp = reinterpret_cast<VD *>(nv);
  F *fp = reinterpret_cast<F *>(nv);
  D *dp = reinterpret_cast<D *>(nv);
  HH *hhp = reinterpret_cast<HH *>(nv);
  volatile int one = 1;
  V va[2];
  MW m;
  H h;
  F f;
  D d;
  MW ma[2];
  HH hh;
  COUNT("vcall", vcall(vp));
  COUNT("vdcall", vdcall(vp));
  COUNT("vsub", vsub(vp));
  COUNT("vsubi", vsubi(vp, one));
  COUNT("varith", varith(vp));
  COUNT("varithi", varithi(vp, one));
  COUNT("vbase", vbase(mp));
  COUNT("vmemarr", vmemarr(hp));
  COUNT("vmemdecay", vmemdecay(hp));
  COUNT("vdtor", vdtor(vdp));
  COUNT("vmember", vmember(hp));
  COUNT("final", final(fp));
  COUNT("qual", qual(vp));
  COUNT("dfinal", dfinal(dp));
  COUNT("nonvirt", nonvirt(vp));
  COUNT("nonnull", vcall(va); vdcall(va); vsub(va); vsubi(va, one);
        varith(va); varithi(va, one); vbase(&m); vmemarr(&h); vmemdecay(&h);
        vmember(&h); final(&f); qual(va); dfinal(&d); nonvirt(va));
  COUNT("vdbase", vdbase(mp));
  COUNT("vsubbase", vsubbase(mp));
  COUNT("varithbase", varithbase(mp));
  COUNT("vthis", vthis(mp));
  COUNT("vlambda", vlambda(mp));
  COUNT("vref", vref(mp));
  COUNT("vnested", vnested(hhp));
  COUNT("nonnull_gcc", vdbase(ma); vsubbase(ma); varithbase(ma); vthis(ma);
        vlambda(ma); vref(ma); vnested(&hh));
  return 0;
}

// ALL: vcall 1
// ALL-NEXT: vdcall 1
// ALL-NEXT: vsub 1
// ALL-NEXT: vsubi 1
// ALL-NEXT: varith 1
// ALL-NEXT: varithi 1
// ALL-NEXT: vbase 1
// ALL-NEXT: vmemarr 1
// ALL-NEXT: vmemdecay 1
// ALL-NEXT: vdtor 1
// ALL-NEXT: vmember 1
// ALL-NEXT: final 1
// ALL-NEXT: qual 1
// ALL-NEXT: dfinal 1
// ALL-NEXT: nonvirt 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: vdbase 1
// ALL-NEXT: vsubbase 1
// ALL-NEXT: varithbase 1
// ALL-NEXT: vthis 1
// ALL-NEXT: vlambda 1
// ALL-NEXT: vref 1
// ALL-NEXT: vnested 1
// ALL-NEXT: nonnull_gcc 0

// IND: vcall 0
// IND-NEXT: vdcall 0
// IND-NEXT: vsub 0
// IND-NEXT: vsubi 0
// IND-NEXT: varith 0
// IND-NEXT: varithi 0
// IND-NEXT: vbase 0
// IND-NEXT: vmemarr 0
// IND-NEXT: vmemdecay 0
// IND-NEXT: vdtor 0
// IND-NEXT: vmember 0
// IND-NEXT: final 0
// IND-NEXT: qual 1
// IND-NEXT: dfinal 0
// IND-NEXT: nonvirt 1
// IND-NEXT: nonnull 0
// IND-NEXT: vdbase 0
// IND-NEXT: vsubbase 0
// IND-NEXT: varithbase 0
// IND-NEXT: vthis 1
// IND-NEXT: vlambda 1
// IND-NEXT: vref 1
// IND-NEXT: vnested 0
// IND-NEXT: nonnull_gcc 0

// ACC: vcall 1
// ACC-NEXT: vdcall 1
// ACC-NEXT: vsub 1
// ACC-NEXT: vsubi 1
// ACC-NEXT: varith 1
// ACC-NEXT: varithi 1
// ACC-NEXT: vbase 1
// ACC-NEXT: vmemarr 1
// ACC-NEXT: vmemdecay 1
// ACC-NEXT: vdtor 1
// ACC-NEXT: vmember 1
// ACC-NEXT: final 1
// ACC-NEXT: qual 0
// ACC-NEXT: dfinal 1
// ACC-NEXT: nonvirt 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: vdbase 1
// ACC-NEXT: vsubbase 1
// ACC-NEXT: varithbase 1
// ACC-NEXT: vthis 0
// ACC-NEXT: vlambda 0
// ACC-NEXT: vref 0
// ACC-NEXT: vnested 1
// ACC-NEXT: nonnull_gcc 0
