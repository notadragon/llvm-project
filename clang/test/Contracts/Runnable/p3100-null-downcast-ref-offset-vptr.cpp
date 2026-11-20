// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.vptr0
// RUN: %t.vptr0 | FileCheck %s --check-prefix=VPTR
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.vptr2
// RUN: %t.vptr2 | FileCheck %s --check-prefix=VPTR
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.nvptr0
// RUN: %t.nvptr0 | FileCheck %s --check-prefix=NVPTR0
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w -O2 \
// RUN:   -fsanitize=null,vptr -fsanitize-semantic=null:noexcept_observe -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.nvptr2
// RUN: %t.nvptr2 | FileCheck %s --check-prefix=NVPTR2
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 -w \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:ignore \
// RUN:   %libcxx_flags -o %t.ign0
// RUN: %t.ign0 | FileCheck %s --check-prefix=IGN

// P3100: with -fsanitize=vptr routed to the contract-violation handler as
// well as ub:expr.unary.dereference.nullptr configured (and
// -fsanitize=null too), a downcast of `*p` to a reference, `p` null, with
// the base at a nonzero offset in the derived class, used -- read, stored
// to, bound, its member's address taken, called through -- is reported
// once, its vptr checks (the downcast's and the use's) skipped on the null
// pointer (DECISIONS.md E116 (a); CLANG-806: they read the vtable
// pointer through the derived object's address, not null, and faulted
// before any report).  The forms are those of
// p3100-null-downcast-ref-offset.cpp; gnu_gcc's mirror is
// g++.dg/ubsan/p3100-null-downcast-ref-offset-vptr*.C (GCC-872).  With
// -fsanitize=null too, the forms at offset zero are reported twice
// (CLANG-796, accepted under DECISIONS.md E120) and the `*this` control
// twice at -O0 (CLANG-792, DECISIONS.md E114 (b)).  With the group
// ignored, nothing is reported and the vptr checks are stock: at a nonzero
// offset they read through the derived object's address and fault.  A
// fault is caught as SIGSEGV and resumed with siglongjmp.

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

// VPTR: drd 1
// VPTR-NEXT: dbind 1
// VPTR-NEXT: dcall 1
// VPTR-NEXT: srd 1
// VPTR-NEXT: sbind 1
// VPTR-NEXT: scall 1
// VPTR-NEXT: drd0 1
// VPTR-NEXT: dbind0 1
// VPTR-NEXT: daddr 1
// VPTR-NEXT: dst 1
// VPTR-NEXT: drv 1
// VPTR-NEXT: dvcall 1
// VPTR-NEXT: dhcall 1
// VPTR-NEXT: dxcall 1
// VPTR-NEXT: dthis 1
// VPTR-NEXT: nonnull 0

// NVPTR0: drd 1
// NVPTR0-NEXT: dbind 1
// NVPTR0-NEXT: dcall 1
// NVPTR0-NEXT: srd 1
// NVPTR0-NEXT: sbind 1
// NVPTR0-NEXT: scall 1
// NVPTR0-NEXT: drd0 2
// NVPTR0-NEXT: dbind0 2
// NVPTR0-NEXT: daddr 1
// NVPTR0-NEXT: dst 1
// NVPTR0-NEXT: drv 1
// NVPTR0-NEXT: dvcall 1
// NVPTR0-NEXT: dhcall 1
// NVPTR0-NEXT: dxcall 1
// NVPTR0-NEXT: dthis 2
// NVPTR0-NEXT: nonnull 0

// NVPTR2: drd 1
// NVPTR2-NEXT: dbind 1
// NVPTR2-NEXT: dcall 1
// NVPTR2-NEXT: srd 1
// NVPTR2-NEXT: sbind 1
// NVPTR2-NEXT: scall 1
// NVPTR2-NEXT: drd0 2
// NVPTR2-NEXT: dbind0 2
// NVPTR2-NEXT: daddr 1
// NVPTR2-NEXT: dst 1
// NVPTR2-NEXT: drv 1
// NVPTR2-NEXT: dvcall 1
// NVPTR2-NEXT: dhcall 1
// NVPTR2-NEXT: dxcall 1
// NVPTR2-NEXT: dthis 1
// NVPTR2-NEXT: nonnull 0

// IGN: drd 0
// IGN-NEXT: dbind 0
// IGN-NEXT: dcall 0
// IGN-NEXT: srd 0
// IGN-NEXT: sbind 0
// IGN-NEXT: scall 0
// IGN-NEXT: drd0 0
// IGN-NEXT: dbind0 0
// IGN-NEXT: daddr 0
// IGN-NEXT: dst 0
// IGN-NEXT: drv 0
// IGN-NEXT: dvcall 0
// IGN-NEXT: dhcall 0
// IGN-NEXT: dxcall 0
// IGN-NEXT: dthis 0
// IGN-NEXT: nonnull 0
