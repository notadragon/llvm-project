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
// RUN: %t.san0 | FileCheck %s --check-prefix=SAN0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null -fsanitize-semantic=null:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.san2
// RUN: %t.san2 | FileCheck %s --check-prefix=SAN2

// P3100: a downcast of `*p` to a reference, `p` null, is checked by the
// use of the downcast object -- a read of its member or a store to it
// (.access), a reference bound to it, the address of its member, a
// non-virtual member call on it (.indirection), a virtual call on it
// (.access, DECISIONS.md E110 (a)) -- on p itself, also where the base is
// at a nonzero offset in the derived class, so that the derived object's
// address is not null (CLANG-802: nothing was reported there).
// Polymorphic (`V` after the polymorphic `X` in `D2`) and not (`S` after
// `Y` in `DS2`).  The virtual calls are of the derived class's own
// function, of the base's and of the other base's; a reference is also
// bound by an rvalue downcast.  The controls: the base at offset zero, a
// downcast of `*this` in a member function called through the null
// pointer (reported once, at the call: DECISIONS.md E100 (a)), non-null
// pointers.  With -fsanitize=null routed too, each at a nonzero offset
// once: the sanitizer's null check of the downcast (stock) tests the
// derived object's address, not null.  (At offset zero, twice: CLANG-796,
// accepted under DECISIONS.md E120, pinned in
// p3100-null-downcast-ref-sanitize-null.cpp; the `*this` control twice at
// -O0, the callee's prologue check, CLANG-792, DECISIONS.md E114 (b).)  A
// fault after the report is caught as SIGSEGV and resumed with siglongjmp.
// gnu_gcc's mirror: g++.dg/contracts/cpp26/p3100-null-downcast-ref-offset.h
// (GCC-869, GCC-870; CLANG-805).  With -fsanitize=vptr routed:
// p3100-null-downcast-ref-offset-vptr.cpp.

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
  void tc();
  int v;
};
struct X {
  virtual void x() {}
  long pad;
};
struct D2 : X, V {
  int d;
  void dn() {}
  virtual void dv() {}
};
struct S { int s; };
struct Y { long y; };
struct DS2 : Y, S {
  int t;
  void tn() {}
};
struct D : V { int d; };
NI int drd(V *vp) { return static_cast<D2 &>(*vp).d; }
NI D2 &dbind(V *vp) { return static_cast<D2 &>(*vp); }
NI void dcall(V *vp) { static_cast<D2 &>(*vp).dn(); }
NI int srd(S *sp) { return static_cast<DS2 &>(*sp).t; }
NI DS2 &sbind(S *sp) { return static_cast<DS2 &>(*sp); }
NI void scall(S *sp) { static_cast<DS2 &>(*sp).tn(); }
NI int drd0(V *vp) { return static_cast<D &>(*vp).d; }
NI D &dbind0(V *vp) { return static_cast<D &>(*vp); }
NI int *daddr(V *vp) { return &static_cast<D2 &>(*vp).d; }
NI void dst(V *vp) { static_cast<D2 &>(*vp).d = 1; }
NI D2 &&drv(V *vp) { return static_cast<D2 &&>(*vp); }
NI void dvcall(V *vp) { static_cast<D2 &>(*vp).dv(); }
NI void dhcall(V *vp) { static_cast<D2 &>(*vp).h(); }
NI void dxcall(V *vp) { static_cast<D2 &>(*vp).x(); }
NI void V::tc() { static_cast<D2 &>(*this).dn(); }
NI void dthis(V *vp) { vp->tc(); }

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
  S *sp = reinterpret_cast<S *>(nv);
  D2 d2;
  DS2 ds2{};
  COUNT("drd", drd(vp));
  COUNT("dbind", dbind(vp));
  COUNT("dcall", dcall(vp));
  COUNT("srd", srd(sp));
  COUNT("sbind", sbind(sp));
  COUNT("scall", scall(sp));
  COUNT("drd0", drd0(vp));
  COUNT("dbind0", dbind0(vp));
  COUNT("daddr", daddr(vp));
  COUNT("dst", dst(vp));
  COUNT("drv", drv(vp));
  COUNT("dvcall", dvcall(vp));
  COUNT("dhcall", dhcall(vp));
  COUNT("dxcall", dxcall(vp));
  COUNT("dthis", dthis(vp));
  COUNT("nonnull", drd(&d2); dbind(&d2); dcall(&d2); srd(&ds2); scall(&ds2);
        daddr(&d2); dst(&d2); dvcall(&d2); dhcall(&d2); dxcall(&d2);
        dthis(&d2));
  return 0;
}

// ALL: drd 1
// ALL-NEXT: dbind 1
// ALL-NEXT: dcall 1
// ALL-NEXT: srd 1
// ALL-NEXT: sbind 1
// ALL-NEXT: scall 1
// ALL-NEXT: drd0 1
// ALL-NEXT: dbind0 1
// ALL-NEXT: daddr 1
// ALL-NEXT: dst 1
// ALL-NEXT: drv 1
// ALL-NEXT: dvcall 1
// ALL-NEXT: dhcall 1
// ALL-NEXT: dxcall 1
// ALL-NEXT: dthis 1
// ALL-NEXT: nonnull 0

// IND: drd 0
// IND-NEXT: dbind 1
// IND-NEXT: dcall 1
// IND-NEXT: srd 0
// IND-NEXT: sbind 1
// IND-NEXT: scall 1
// IND-NEXT: drd0 0
// IND-NEXT: dbind0 1
// IND-NEXT: daddr 1
// IND-NEXT: dst 0
// IND-NEXT: drv 1
// IND-NEXT: dvcall 0
// IND-NEXT: dhcall 0
// IND-NEXT: dxcall 0
// IND-NEXT: dthis 1
// IND-NEXT: nonnull 0

// ACC: drd 1
// ACC-NEXT: dbind 0
// ACC-NEXT: dcall 0
// ACC-NEXT: srd 1
// ACC-NEXT: sbind 0
// ACC-NEXT: scall 0
// ACC-NEXT: drd0 1
// ACC-NEXT: dbind0 0
// ACC-NEXT: daddr 0
// ACC-NEXT: dst 1
// ACC-NEXT: drv 0
// ACC-NEXT: dvcall 1
// ACC-NEXT: dhcall 1
// ACC-NEXT: dxcall 1
// ACC-NEXT: dthis 0
// ACC-NEXT: nonnull 0

// SAN0: drd 1
// SAN0-NEXT: dbind 1
// SAN0-NEXT: dcall 1
// SAN0-NEXT: srd 1
// SAN0-NEXT: sbind 1
// SAN0-NEXT: scall 1
// SAN0-NEXT: drd0 2
// SAN0-NEXT: dbind0 2
// SAN0-NEXT: daddr 1
// SAN0-NEXT: dst 1
// SAN0-NEXT: drv 1
// SAN0-NEXT: dvcall 1
// SAN0-NEXT: dhcall 1
// SAN0-NEXT: dxcall 1
// SAN0-NEXT: dthis 2
// SAN0-NEXT: nonnull 0

// SAN2: drd 1
// SAN2-NEXT: dbind 1
// SAN2-NEXT: dcall 1
// SAN2-NEXT: srd 1
// SAN2-NEXT: sbind 1
// SAN2-NEXT: scall 1
// SAN2-NEXT: drd0 2
// SAN2-NEXT: dbind0 2
// SAN2-NEXT: daddr 1
// SAN2-NEXT: dst 1
// SAN2-NEXT: drv 1
// SAN2-NEXT: dvcall 1
// SAN2-NEXT: dhcall 1
// SAN2-NEXT: dxcall 1
// SAN2-NEXT: dthis 1
// SAN2-NEXT: nonnull 0
