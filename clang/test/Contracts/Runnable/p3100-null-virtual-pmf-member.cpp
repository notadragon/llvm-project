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

// P3100: a call through a pointer to member function on a member
// subobject of the object a null pointer points to -- `(hp->m.*p)()`,
// `(hp->arr[1].*p)()`, `((hp->*pm).*p)()`, `((*hp).m.*p)()` -- is reported
// once, under the leaf the pointer to member function selects: through a
// virtual one the call reads the subobject's vtable pointer through hp,
// .access, as one on `*vp` does (DECISIONS.md E102 (a),
// Runnable/p3100-null-virtual-pmf.cpp); through a non-virtual one,
// .indirection.  Which it is only the pointer's value says, so the member
// access's null check of hp is made after the pointer to member function
// is evaluated.  A non-null pointer is not reported.  (CLANG-788: the
// virtual forms were .indirection.)  Then gnu_gcc's: on the member of an
// element at a constant subscript, `(hp[1].m.*p)()`, and on an element of
// a member array through a pointer to data member, `((hp->*pa)[1].*p)()`,
// the same; on a member through `this` in a member function called
// through a null pointer (DECISIONS.md E106 (a)) or through a reference
// bound to `*hp`, only the call or the binding is reported, once under
// .indirection.  Then Clang's: on a member of a base class at a nonzero
// offset, `(dp->m.*p)()` and `((*dp).m.*p)()`, the same as on a member;
// on a member of a virtual base, `(vp->m.*p)()`, once under .access
// whatever the pointer to member function is, the base's offset being
// read through vp first (DECISIONS.md E94 (b)).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3100-null-virtual-pmf-member*.C, whose header
// p3100-null-virtual-pmf-member.h has the same forms in this order,
// Clang's included.

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
typedef void (V::*VPM)();
struct H {
  int x;
  V m;
  V arr[2];
  NI void self(VPM p) { (m.*p)(); }
};
NI void mem(H *hp, VPM p) { (hp->m.*p)(); }
NI void memarr(H *hp, VPM p) { (hp->arr[1].*p)(); }
NI void memptr(H *hp, V H::*pm, VPM p) { ((hp->*pm).*p)(); }
NI void dmem(H *hp, VPM p) { ((*hp).m.*p)(); }
// gnu_gcc's: through `this` (DECISIONS.md E106 (a)) and a reference, on an
// element at a constant subscript, and through a pointer to data member
// of a member.
NI void selfm(H *hp, VPM p) { hp->self(p); }
NI void refbody(H &r, VPM p) { (r.m.*p)(); }
NI void refm(H *hp, VPM p) { refbody(*hp, p); }
NI void submem(H *hp, VPM p) { (hp[1].m.*p)(); }
NI void ptrmemarr(H *hp, V (H::*pa)[2], VPM p) { ((hp->*pa)[1].*p)(); }
// Clang's: a member of a base class at a nonzero offset, of a virtual
// base.
struct X { long pad; };
struct B { int y; V m; };
struct D : X, B {};
struct VD : virtual B { int z; };
NI void basemem(D *dp, VPM p) { (dp->m.*p)(); }
NI void dbasemem(D *dp, VPM p) { ((*dp).m.*p)(); }
NI void vbasemem(VD *vp, VPM p) { (vp->m.*p)(); }

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
  H *hp = reinterpret_cast<H *>(nv);
  D *dp = reinterpret_cast<D *>(nv);
  VD *vp = reinterpret_cast<VD *>(nv);
  H h;
  H ha[2];
  D d;
  VD vd;
  COUNT("memv", mem(hp, &V::h));
  COUNT("memnv", mem(hp, &V::nv));
  COUNT("memarrv", memarr(hp, &V::h));
  COUNT("memarrnv", memarr(hp, &V::nv));
  COUNT("memptrv", memptr(hp, &H::m, &V::h));
  COUNT("memptrnv", memptr(hp, &H::m, &V::nv));
  COUNT("dmemv", dmem(hp, &V::h));
  COUNT("dmemnv", dmem(hp, &V::nv));
  COUNT("nonnull", mem(&h, &V::h); mem(&h, &V::nv); memarr(&h, &V::h);
        memarr(&h, &V::nv); memptr(&h, &H::m, &V::h);
        memptr(&h, &H::m, &V::nv); dmem(&h, &V::h); dmem(&h, &V::nv));
  COUNT("selfv", selfm(hp, &V::h));
  COUNT("selfnv", selfm(hp, &V::nv));
  COUNT("refv", refm(hp, &V::h));
  COUNT("refnv", refm(hp, &V::nv));
  COUNT("submemv", submem(hp, &V::h));
  COUNT("submemnv", submem(hp, &V::nv));
  COUNT("ptrmemarrv", ptrmemarr(hp, &H::arr, &V::h));
  COUNT("ptrmemarrnv", ptrmemarr(hp, &H::arr, &V::nv));
  COUNT("nonnull_gcc", selfm(ha, &V::h); selfm(ha, &V::nv);
        refm(ha, &V::h); refm(ha, &V::nv); submem(ha, &V::h);
        submem(ha, &V::nv); ptrmemarr(ha, &H::arr, &V::h);
        ptrmemarr(ha, &H::arr, &V::nv));
  COUNT("basememv", basemem(dp, &V::h));
  COUNT("basememnv", basemem(dp, &V::nv));
  COUNT("dbasememv", dbasemem(dp, &V::h));
  COUNT("dbasememnv", dbasemem(dp, &V::nv));
  COUNT("vbasememv", vbasemem(vp, &V::h));
  COUNT("vbasememnv", vbasemem(vp, &V::nv));
  COUNT("nonnull_clang", basemem(&d, &V::h); basemem(&d, &V::nv);
        dbasemem(&d, &V::h); dbasemem(&d, &V::nv); vbasemem(&vd, &V::h);
        vbasemem(&vd, &V::nv));
  return 0;
}

// ALL: memv 1
// ALL-NEXT: memnv 1
// ALL-NEXT: memarrv 1
// ALL-NEXT: memarrnv 1
// ALL-NEXT: memptrv 1
// ALL-NEXT: memptrnv 1
// ALL-NEXT: dmemv 1
// ALL-NEXT: dmemnv 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: selfv 1
// ALL-NEXT: selfnv 1
// ALL-NEXT: refv 1
// ALL-NEXT: refnv 1
// ALL-NEXT: submemv 1
// ALL-NEXT: submemnv 1
// ALL-NEXT: ptrmemarrv 1
// ALL-NEXT: ptrmemarrnv 1
// ALL-NEXT: nonnull_gcc 0
// ALL-NEXT: basememv 1
// ALL-NEXT: basememnv 1
// ALL-NEXT: dbasememv 1
// ALL-NEXT: dbasememnv 1
// ALL-NEXT: vbasememv 1
// ALL-NEXT: vbasememnv 1
// ALL-NEXT: nonnull_clang 0

// IND: memv 0
// IND-NEXT: memnv 1
// IND-NEXT: memarrv 0
// IND-NEXT: memarrnv 1
// IND-NEXT: memptrv 0
// IND-NEXT: memptrnv 1
// IND-NEXT: dmemv 0
// IND-NEXT: dmemnv 1
// IND-NEXT: nonnull 0
// IND-NEXT: selfv 1
// IND-NEXT: selfnv 1
// IND-NEXT: refv 1
// IND-NEXT: refnv 1
// IND-NEXT: submemv 0
// IND-NEXT: submemnv 1
// IND-NEXT: ptrmemarrv 0
// IND-NEXT: ptrmemarrnv 1
// IND-NEXT: nonnull_gcc 0
// IND-NEXT: basememv 0
// IND-NEXT: basememnv 1
// IND-NEXT: dbasememv 0
// IND-NEXT: dbasememnv 1
// IND-NEXT: vbasememv 0
// IND-NEXT: vbasememnv 0
// IND-NEXT: nonnull_clang 0

// ACC: memv 1
// ACC-NEXT: memnv 0
// ACC-NEXT: memarrv 1
// ACC-NEXT: memarrnv 0
// ACC-NEXT: memptrv 1
// ACC-NEXT: memptrnv 0
// ACC-NEXT: dmemv 1
// ACC-NEXT: dmemnv 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: selfv 0
// ACC-NEXT: selfnv 0
// ACC-NEXT: refv 0
// ACC-NEXT: refnv 0
// ACC-NEXT: submemv 1
// ACC-NEXT: submemnv 0
// ACC-NEXT: ptrmemarrv 1
// ACC-NEXT: ptrmemarrnv 0
// ACC-NEXT: nonnull_gcc 0
// ACC-NEXT: basememv 1
// ACC-NEXT: basememnv 0
// ACC-NEXT: dbasememv 1
// ACC-NEXT: dbasememnv 0
// ACC-NEXT: vbasememv 1
// ACC-NEXT: vbasememnv 1
// ACC-NEXT: nonnull_clang 0
