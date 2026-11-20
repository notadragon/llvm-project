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
// RUN: %t.ind | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2
// RUN: %t.ind2 | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2
// RUN: %t.acc2 | FileCheck %s --check-prefix=ACC
// P3100: a glvalue that is already a reference -- a reference variable or
// parameter, a reference member, a call returning a reference -- indirects
// through nothing (DECISIONS.md E92 (a)).  One null dereference, `*p` bound
// to a reference, is reported once, under .indirection, however the
// reference is then used: bound to another reference (`const int &r2 = r`,
// `int &&r2 = std::move(r)`, `static_cast<int &&>(r)`, `return r` from a
// function returning a reference, passed to a reference parameter),
// converted to a base (also a virtual one, or an empty one at a nonzero
// offset) or to a pointer to a base, read, copied, stored to, a member
// function called on it (also through a reference parameter or member, and
// an empty base's), its member read, discarded or its address taken, or the
// same through the reference a call returns.  No access through a
// reference is checked, at any optimization level.  A non-null pointer is
// not reported.
// (CLANG-767: the binding of a reference to a reference, the conversion
// of one to a base, the copy of a class through one and, at -O0, an access
// through the reference a call returns were checked again.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-reference-rebind.h, these
// forms in this order, each reported the same.)

#include <contracts>
#include <csetjmp>
#include <csignal>
#include <cstdio>
#include <utility>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &v) {
  if (v.kind() == cs::assertion_kind::implicit)
    ++reports;
}

static sigjmp_buf env;
static void on_segv(int) { siglongjmp(env, 1); }

#define NI __attribute__((noinline))
struct S {
  int m;
  NI void f() {}
};
struct A { int a; };
struct B { int b; };
struct D : A, B {};
struct V : virtual B { int v; };
// GCC's: an empty base at a nonzero offset (a member of type E is at
// offset 0).
struct E {
  NI void g() {}
};
struct F0 { E e; int i; };
struct DEn : F0, E {};

NI int &id(int &r) { return r; }
NI S &sid(S &r) { return r; }
NI D &did(D &r) { return r; }
// A reference bound to a reference.
NI int &rret(int *p) { int &r = *p; return r; }
NI int rebind_call_arg(int *p) { int &r = *p; return id(r); }
NI int rebind_local(int *p) { int &r = *p; const int &r2 = r; return r2; }
NI bool rebind_noread(int *p) { int &r = *p; int &r2 = r; return &r2; }
NI int rebind_move(int *p) { int &r = *p; int &&r2 = std::move(r); return r2; }
NI int rebind_cast(int *p) {
  int &r = *p;
  int &&r2 = static_cast<int &&>(r);
  return r2;
}
// A reference converted to a base.
NI bool rebind_base(D *dp) { D &d = *dp; B &b = d; return &b; }
NI bool rebind_vbase(V *vp) { V &v = *vp; B &b = v; return &b; }
NI bool rcall_base(D *dp) { B &b = did(*dp); return &b; }
// An access through a reference.
NI int iread_ref(int *p) { int &r = *p; return r; }
NI void scopy_ref(S *sp) { S &r = *sp; S s2 = r; (void)s2; }
NI int rcall_member(S *sp) { return sid(*sp).m; }
NI void rcall_call(S *sp) { sid(*sp).f(); }
NI void rcall_store(int *p) { id(*p) = 1; }
NI void rcall_copy(S *sp) { S s2 = sid(*sp); (void)s2; }
// GCC's extra forms: a member function called on a reference, the address
// of its member, its conversion to a base pointer, a discarded member, an
// empty base at a nonzero offset.
NI void rvar_call(S *sp) { S &r = *sp; r.f(); }
NI int *rvar_addr(S *sp) { S &r = *sp; return &r.m; }
NI B *rvar_ptr(D *dp) { D &d = *dp; return &d; }
NI void rparam_call(S &r) { r.f(); }
NI void rmember_call(S *sp) { struct R { S &r; } x{*sp}; x.r.f(); }
NI void rvar_disc(S *sp) { S &r = *sp; r.m; }
NI bool rebind_ebase(DEn *dp) { DEn &d = *dp; E &e = d; return &e; }
NI void rvar_ebase_call(DEn *dp) { DEn &d = *dp; d.E::g(); }

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
  int *volatile vp = nullptr;
  int *ip = vp;
  S *sp = reinterpret_cast<S *>(vp);
  D *dp = reinterpret_cast<D *>(vp);
  V *vbp = reinterpret_cast<V *>(vp);
  DEn *denp = reinterpret_cast<DEn *>(vp);
  int i = 0;
  S s{};
  D d{};
  V v;
  DEn den{};
  COUNT("rret", (void)rret(ip));
  COUNT("rebind_call_arg", rebind_call_arg(ip));
  COUNT("rebind_local", rebind_local(ip));
  COUNT("rebind_noread", rebind_noread(ip));
  COUNT("rebind_move", rebind_move(ip));
  COUNT("rebind_cast", rebind_cast(ip));
  COUNT("rebind_base", rebind_base(dp));
  COUNT("rebind_vbase", rebind_vbase(vbp));
  COUNT("rcall_base", rcall_base(dp));
  COUNT("iread_ref", iread_ref(ip));
  COUNT("scopy_ref", scopy_ref(sp));
  COUNT("rcall_member", rcall_member(sp));
  COUNT("rcall_call", rcall_call(sp));
  COUNT("rcall_store", rcall_store(ip));
  COUNT("rcall_copy", rcall_copy(sp));
  COUNT("nonnull", (void)rret(&i); rebind_call_arg(&i); rebind_local(&i);
        rebind_move(&i); rebind_base(&d); rebind_vbase(&v); rcall_base(&d);
        scopy_ref(&s); rcall_member(&s); rcall_call(&s); rcall_store(&i);
        rcall_copy(&s));
  COUNT("rvar_call", rvar_call(sp));
  COUNT("rvar_addr", rvar_addr(sp));
  COUNT("rvar_ptr", rvar_ptr(dp));
  COUNT("rparam_call", rparam_call(*sp));
  COUNT("rmember_call", rmember_call(sp));
  COUNT("rvar_disc", rvar_disc(sp));
  COUNT("rebind_ebase", rebind_ebase(denp));
  COUNT("rvar_ebase_call", rvar_ebase_call(denp));
  COUNT("nonnull_extra", rvar_call(&s); rvar_addr(&s); rvar_ptr(&d);
        rparam_call(s); rmember_call(&s); rvar_disc(&s); rebind_ebase(&den);
        rvar_ebase_call(&den));
  return 0;
}

// ALL: rret 1
// ALL-NEXT: rebind_call_arg 1
// ALL-NEXT: rebind_local 1
// ALL-NEXT: rebind_noread 1
// ALL-NEXT: rebind_move 1
// ALL-NEXT: rebind_cast 1
// ALL-NEXT: rebind_base 1
// ALL-NEXT: rebind_vbase 1
// ALL-NEXT: rcall_base 1
// ALL-NEXT: iread_ref 1
// ALL-NEXT: scopy_ref 1
// ALL-NEXT: rcall_member 1
// ALL-NEXT: rcall_call 1
// ALL-NEXT: rcall_store 1
// ALL-NEXT: rcall_copy 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: rvar_call 1
// ALL-NEXT: rvar_addr 1
// ALL-NEXT: rvar_ptr 1
// ALL-NEXT: rparam_call 1
// ALL-NEXT: rmember_call 1
// ALL-NEXT: rvar_disc 1
// ALL-NEXT: rebind_ebase 1
// ALL-NEXT: rvar_ebase_call 1
// ALL-NEXT: nonnull_extra 0

// ACC: rret 0
// ACC-NEXT: rebind_call_arg 0
// ACC-NEXT: rebind_local 0
// ACC-NEXT: rebind_noread 0
// ACC-NEXT: rebind_move 0
// ACC-NEXT: rebind_cast 0
// ACC-NEXT: rebind_base 0
// ACC-NEXT: rebind_vbase 0
// ACC-NEXT: rcall_base 0
// ACC-NEXT: iread_ref 0
// ACC-NEXT: scopy_ref 0
// ACC-NEXT: rcall_member 0
// ACC-NEXT: rcall_call 0
// ACC-NEXT: rcall_store 0
// ACC-NEXT: rcall_copy 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: rvar_call 0
// ACC-NEXT: rvar_addr 0
// ACC-NEXT: rvar_ptr 0
// ACC-NEXT: rparam_call 0
// ACC-NEXT: rmember_call 0
// ACC-NEXT: rvar_disc 0
// ACC-NEXT: rebind_ebase 0
// ACC-NEXT: rvar_ebase_call 0
// ACC-NEXT: nonnull_extra 0
