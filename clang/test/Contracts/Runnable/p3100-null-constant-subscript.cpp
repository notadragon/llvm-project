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
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -DNONCONST \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.allnc
// RUN: %t.allnc | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 -DNONCONST \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t.all2nc
// RUN: %t.all2nc | FileCheck %s --check-prefix=ALL
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -DNONCONST \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.indnc
// RUN: %t.indnc | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 -DNONCONST \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind2nc
// RUN: %t.ind2nc | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -DNONCONST \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.accnc
// RUN: %t.accnc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 -DNONCONST \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc2nc
// RUN: %t.acc2nc | FileCheck %s --check-prefix=ACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t.wrap
// RUN: %t.wrap | FileCheck %s --check-prefix=WRAP
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t.wrap2
// RUN: %t.wrap2 | FileCheck %s --check-prefix=WRAP
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t.wrapind
// RUN: %t.wrapind | FileCheck %s --check-prefix=WRAPIND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t.wrapind2
// RUN: %t.wrapind2 | FileCheck %s --check-prefix=WRAPIND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t.wrapacc
// RUN: %t.wrapacc | FileCheck %s --check-prefix=WRAPACC
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t.wrapacc2
// RUN: %t.wrapacc2 | FileCheck %s --check-prefix=WRAPACC

// P3100: the element at a subscript of a null pointer, `sp[1]` (also
// negative, `sp[-1]`, and a non-constant index, the NONCONST runs), is
// checked on the pointer itself, as `sp[0]` is, once whatever the
// optimization level (DECISIONS.md E97 (c) for a constant subscript, E83
// (c) for any): a member read or stored to (`sp[1].a`, an element of an
// array member), a scalar element read or stored to, a class copied from or
// stored to, the object of `sp[1].*pm`, a member read through a vtable
// pointer and a virtual member function called (which reads the vtable
// pointer, DECISIONS.md E102 (a)) are reported once under .access; the
// address of the element or of its member, a reference bound to either, a
// member function called on it and an empty class copied from or stored
// to it access nothing and are reported once under .indirection
// (DECISIONS.md E86 (a)).
// A non-null pointer is not reported.  Then Clang's extra forms, each
// .indirection: a member function called through a pointer to member
// function, a reference bound to the element as an argument, returned,
// const, captured by a lambda, a non-trivial destructor called, a
// reference bound to a base of the element and a member function of a
// base called on it.  With ub:expr.add.out.of.bounds.pointer.arithmetic
// configured too (the WRAP runs, and WRAPIND and WRAPACC with each leaf),
// every form is checked for null on the pointer as without it, once, under
// the leaf it has without it; `sp[-1]` also wraps around the address
// space, reported by that check too, so its forms report once more.
// (CLANG-781: `vcall` was .indirection.)
// (CLANG-777: a use that checks the element's address -- the reference
// bindings and member calls -- checked `sp + 1`, so was not reported, and
// with a non-constant subscript was reported at -O2 only.)
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-constant-subscript*.C,
// whose header has the forms up to `nonnull` in this order; GCC checks a
// non-constant subscript on the element's address, the CLANG-745
// divergence.  Clang's
// extra forms follow it there, after `nonnull`.  Its -wrap runs are the
// WRAP runs, its -wrap-{indirection,access} runs the WRAPIND and WRAPACC
// runs.)

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

struct E {};
struct S { int a, b; int arr[2]; void f() {} };
struct V { virtual void h() {} int v; };
struct T { int a; ~T() {} };
struct B { int x; void g() {} };
struct D : B { int y; };

#ifdef NONCONST
static volatile int one = 1, mone = -1;
#define C1 one
#define CM1 mone
#else
#define C1 1
#define CM1 -1
#endif

#define NI __attribute__((noinline))
NI void sbyval(S) {}
NI void ebyval(E) {}
NI void sbyref(S &) {}
NI int mread(S *sp) { return sp[C1].a; }
NI void mstore(S *sp) { sp[C1].a = 1; }
NI int *maddr(S *sp) { return &sp[C1].a; }
NI void mbind(S *sp) { int &r = sp[C1].a; (void)r; }
NI void sbind(S *sp) { S &r = sp[C1]; (void)r; }
NI S *saddr(S *sp) { return &sp[C1]; }
NI void scopy(S *sp) { S s2 = sp[C1]; (void)s2; }
NI void sfrom(S *sp, S &s) { s = sp[C1]; }
NI void sstore(S *sp, S &s) { sp[C1] = s; }
NI void sarg(S *sp) { sbyval(sp[C1]); }
NI void scall(S *sp) { sp[C1].f(); }
NI int arrread(S *sp) { return sp[C1].arr[1]; }
NI void arrstore(S *sp) { sp[C1].arr[1] = 1; }
NI int iread(int *ip) { return ip[C1]; }
NI void istore(int *ip) { ip[C1] = 2; }
NI int pm(S *sp, int S::*p) { return sp[C1].*p; }
NI void efrom(E *ep, E &e) { e = ep[C1]; }
NI void ecopy(E *ep) { E e2 = ep[C1]; (void)e2; }
NI void earg(E *ep) { ebyval(ep[C1]); }
NI void estore(E *ep, E &e) { ep[C1] = e; }
NI void ebind(E *ep) { E &r = ep[C1]; (void)r; }
NI void vcall(V *vp) { vp[C1].h(); }
NI int vread(V *vp) { return vp[C1].v; }
// A negative subscript.
NI int mread_neg(S *sp) { return sp[CM1].a; }
NI int *maddr_neg(S *sp) { return &sp[CM1].a; }
NI void sbind_neg(S *sp) { S &r = sp[CM1]; (void)r; }
NI void ecopy_neg(E *ep) { E e2 = ep[CM1]; (void)e2; }
// Clang's: other uses that bind or call through the element.
NI void pmf(S *sp, void (S::*p)()) { (sp[C1].*p)(); }
NI void sarg_ref(S *sp) { sbyref(sp[C1]); }
NI S &sret_ref(S *sp) { return sp[C1]; }
NI void scbind(S *sp) { const S &r = sp[C1]; (void)r; }
NI void slambda(S *sp) { auto l = [&r = sp[C1]] { (void)r; }; l(); }
NI void tdtor(T *tp) { tp[C1].~T(); }
NI void bbind(D *dp) { B &r = dp[C1]; (void)r; }
NI void bcall(D *dp) { dp[C1].g(); }

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
  S *sp = reinterpret_cast<S *>(nv);
  E *ep = reinterpret_cast<E *>(nv);
  V *vp = reinterpret_cast<V *>(nv);
  T *tp = reinterpret_cast<T *>(nv);
  D *dp = reinterpret_cast<D *>(nv);
  int *ip = nv;
  S s{}, sa[3]{};
  E e, ea[3];
  V va[3];
  T ta[3]{};
  D da[3]{};
  int ia[3]{};
  COUNT("mread", mread(sp));
  COUNT("mstore", mstore(sp));
  COUNT("maddr", maddr(sp));
  COUNT("mbind", mbind(sp));
  COUNT("sbind", sbind(sp));
  COUNT("saddr", saddr(sp));
  COUNT("scopy", scopy(sp));
  COUNT("sfrom", sfrom(sp, s));
  COUNT("sstore", sstore(sp, s));
  COUNT("sarg", sarg(sp));
  COUNT("scall", scall(sp));
  COUNT("arrread", arrread(sp));
  COUNT("arrstore", arrstore(sp));
  COUNT("iread", iread(ip));
  COUNT("istore", istore(ip));
  COUNT("pm", pm(sp, &S::b));
  COUNT("efrom", efrom(ep, e));
  COUNT("ecopy", ecopy(ep));
  COUNT("earg", earg(ep));
  COUNT("estore", estore(ep, e));
  COUNT("ebind", ebind(ep));
  COUNT("vcall", vcall(vp));
  COUNT("vread", vread(vp));
  COUNT("mread_neg", mread_neg(sp));
  COUNT("maddr_neg", maddr_neg(sp));
  COUNT("sbind_neg", sbind_neg(sp));
  COUNT("ecopy_neg", ecopy_neg(ep));
  COUNT("nonnull", mread(sa); mstore(sa); maddr(sa); mbind(sa); sbind(sa);
        saddr(sa); scopy(sa); sfrom(sa, s); sstore(sa, s); sarg(sa);
        scall(sa); arrread(sa); arrstore(sa); iread(ia); istore(ia);
        pm(sa, &S::b); efrom(ea, e); ecopy(ea); earg(ea); estore(ea, e);
        ebind(ea); vcall(va); vread(va); mread_neg(sa + 1);
        maddr_neg(sa + 1); sbind_neg(sa + 1); ecopy_neg(ea + 1));
  COUNT("pmf", pmf(sp, &S::f));
  COUNT("sarg_ref", sarg_ref(sp));
  COUNT("sret_ref", sret_ref(sp));
  COUNT("scbind", scbind(sp));
  COUNT("slambda", slambda(sp));
  COUNT("tdtor", tdtor(tp));
  COUNT("bbind", bbind(dp));
  COUNT("bcall", bcall(dp));
  COUNT("nonnull_extra", pmf(sa, &S::f); sarg_ref(sa); sret_ref(sa);
        scbind(sa); slambda(sa); tdtor(ta); bbind(da); bcall(da));
  return 0;
}

// ALL: mread 1
// ALL-NEXT: mstore 1
// ALL-NEXT: maddr 1
// ALL-NEXT: mbind 1
// ALL-NEXT: sbind 1
// ALL-NEXT: saddr 1
// ALL-NEXT: scopy 1
// ALL-NEXT: sfrom 1
// ALL-NEXT: sstore 1
// ALL-NEXT: sarg 1
// ALL-NEXT: scall 1
// ALL-NEXT: arrread 1
// ALL-NEXT: arrstore 1
// ALL-NEXT: iread 1
// ALL-NEXT: istore 1
// ALL-NEXT: pm 1
// ALL-NEXT: efrom 1
// ALL-NEXT: ecopy 1
// ALL-NEXT: earg 1
// ALL-NEXT: estore 1
// ALL-NEXT: ebind 1
// ALL-NEXT: vcall 1
// ALL-NEXT: vread 1
// ALL-NEXT: mread_neg 1
// ALL-NEXT: maddr_neg 1
// ALL-NEXT: sbind_neg 1
// ALL-NEXT: ecopy_neg 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: pmf 1
// ALL-NEXT: sarg_ref 1
// ALL-NEXT: sret_ref 1
// ALL-NEXT: scbind 1
// ALL-NEXT: slambda 1
// ALL-NEXT: tdtor 1
// ALL-NEXT: bbind 1
// ALL-NEXT: bcall 1
// ALL-NEXT: nonnull_extra 0

// IND: mread 0
// IND-NEXT: mstore 0
// IND-NEXT: maddr 1
// IND-NEXT: mbind 1
// IND-NEXT: sbind 1
// IND-NEXT: saddr 1
// IND-NEXT: scopy 0
// IND-NEXT: sfrom 0
// IND-NEXT: sstore 0
// IND-NEXT: sarg 0
// IND-NEXT: scall 1
// IND-NEXT: arrread 0
// IND-NEXT: arrstore 0
// IND-NEXT: iread 0
// IND-NEXT: istore 0
// IND-NEXT: pm 0
// IND-NEXT: efrom 1
// IND-NEXT: ecopy 1
// IND-NEXT: earg 1
// IND-NEXT: estore 1
// IND-NEXT: ebind 1
// IND-NEXT: vcall 0
// IND-NEXT: vread 0
// IND-NEXT: mread_neg 0
// IND-NEXT: maddr_neg 1
// IND-NEXT: sbind_neg 1
// IND-NEXT: ecopy_neg 1
// IND-NEXT: nonnull 0
// IND-NEXT: pmf 1
// IND-NEXT: sarg_ref 1
// IND-NEXT: sret_ref 1
// IND-NEXT: scbind 1
// IND-NEXT: slambda 1
// IND-NEXT: tdtor 1
// IND-NEXT: bbind 1
// IND-NEXT: bcall 1
// IND-NEXT: nonnull_extra 0

// ACC: mread 1
// ACC-NEXT: mstore 1
// ACC-NEXT: maddr 0
// ACC-NEXT: mbind 0
// ACC-NEXT: sbind 0
// ACC-NEXT: saddr 0
// ACC-NEXT: scopy 1
// ACC-NEXT: sfrom 1
// ACC-NEXT: sstore 1
// ACC-NEXT: sarg 1
// ACC-NEXT: scall 0
// ACC-NEXT: arrread 1
// ACC-NEXT: arrstore 1
// ACC-NEXT: iread 1
// ACC-NEXT: istore 1
// ACC-NEXT: pm 1
// ACC-NEXT: efrom 0
// ACC-NEXT: ecopy 0
// ACC-NEXT: earg 0
// ACC-NEXT: estore 0
// ACC-NEXT: ebind 0
// ACC-NEXT: vcall 1
// ACC-NEXT: vread 1
// ACC-NEXT: mread_neg 1
// ACC-NEXT: maddr_neg 0
// ACC-NEXT: sbind_neg 0
// ACC-NEXT: ecopy_neg 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: pmf 0
// ACC-NEXT: sarg_ref 0
// ACC-NEXT: sret_ref 0
// ACC-NEXT: scbind 0
// ACC-NEXT: slambda 0
// ACC-NEXT: tdtor 0
// ACC-NEXT: bbind 0
// ACC-NEXT: bcall 0
// ACC-NEXT: nonnull_extra 0

// WRAP: mread 1
// WRAP-NEXT: mstore 1
// WRAP-NEXT: maddr 1
// WRAP-NEXT: mbind 1
// WRAP-NEXT: sbind 1
// WRAP-NEXT: saddr 1
// WRAP-NEXT: scopy 1
// WRAP-NEXT: sfrom 1
// WRAP-NEXT: sstore 1
// WRAP-NEXT: sarg 1
// WRAP-NEXT: scall 1
// WRAP-NEXT: arrread 1
// WRAP-NEXT: arrstore 1
// WRAP-NEXT: iread 1
// WRAP-NEXT: istore 1
// WRAP-NEXT: pm 1
// WRAP-NEXT: efrom 1
// WRAP-NEXT: ecopy 1
// WRAP-NEXT: earg 1
// WRAP-NEXT: estore 1
// WRAP-NEXT: ebind 1
// WRAP-NEXT: vcall 1
// WRAP-NEXT: vread 1
// WRAP-NEXT: mread_neg 2
// WRAP-NEXT: maddr_neg 2
// WRAP-NEXT: sbind_neg 2
// WRAP-NEXT: ecopy_neg 2
// WRAP-NEXT: nonnull 0
// WRAP-NEXT: pmf 1
// WRAP-NEXT: sarg_ref 1
// WRAP-NEXT: sret_ref 1
// WRAP-NEXT: scbind 1
// WRAP-NEXT: slambda 1
// WRAP-NEXT: tdtor 1
// WRAP-NEXT: bbind 1
// WRAP-NEXT: bcall 1
// WRAP-NEXT: nonnull_extra 0

// WRAPIND: mread 0
// WRAPIND-NEXT: mstore 0
// WRAPIND-NEXT: maddr 1
// WRAPIND-NEXT: mbind 1
// WRAPIND-NEXT: sbind 1
// WRAPIND-NEXT: saddr 1
// WRAPIND-NEXT: scopy 0
// WRAPIND-NEXT: sfrom 0
// WRAPIND-NEXT: sstore 0
// WRAPIND-NEXT: sarg 0
// WRAPIND-NEXT: scall 1
// WRAPIND-NEXT: arrread 0
// WRAPIND-NEXT: arrstore 0
// WRAPIND-NEXT: iread 0
// WRAPIND-NEXT: istore 0
// WRAPIND-NEXT: pm 0
// WRAPIND-NEXT: efrom 1
// WRAPIND-NEXT: ecopy 1
// WRAPIND-NEXT: earg 1
// WRAPIND-NEXT: estore 1
// WRAPIND-NEXT: ebind 1
// WRAPIND-NEXT: vcall 0
// WRAPIND-NEXT: vread 0
// WRAPIND-NEXT: mread_neg 1
// WRAPIND-NEXT: maddr_neg 2
// WRAPIND-NEXT: sbind_neg 2
// WRAPIND-NEXT: ecopy_neg 2
// WRAPIND-NEXT: nonnull 0
// WRAPIND-NEXT: pmf 1
// WRAPIND-NEXT: sarg_ref 1
// WRAPIND-NEXT: sret_ref 1
// WRAPIND-NEXT: scbind 1
// WRAPIND-NEXT: slambda 1
// WRAPIND-NEXT: tdtor 1
// WRAPIND-NEXT: bbind 1
// WRAPIND-NEXT: bcall 1
// WRAPIND-NEXT: nonnull_extra 0

// WRAPACC: mread 1
// WRAPACC-NEXT: mstore 1
// WRAPACC-NEXT: maddr 0
// WRAPACC-NEXT: mbind 0
// WRAPACC-NEXT: sbind 0
// WRAPACC-NEXT: saddr 0
// WRAPACC-NEXT: scopy 1
// WRAPACC-NEXT: sfrom 1
// WRAPACC-NEXT: sstore 1
// WRAPACC-NEXT: sarg 1
// WRAPACC-NEXT: scall 0
// WRAPACC-NEXT: arrread 1
// WRAPACC-NEXT: arrstore 1
// WRAPACC-NEXT: iread 1
// WRAPACC-NEXT: istore 1
// WRAPACC-NEXT: pm 1
// WRAPACC-NEXT: efrom 0
// WRAPACC-NEXT: ecopy 0
// WRAPACC-NEXT: earg 0
// WRAPACC-NEXT: estore 0
// WRAPACC-NEXT: ebind 0
// WRAPACC-NEXT: vcall 1
// WRAPACC-NEXT: vread 1
// WRAPACC-NEXT: mread_neg 2
// WRAPACC-NEXT: maddr_neg 1
// WRAPACC-NEXT: sbind_neg 1
// WRAPACC-NEXT: ecopy_neg 1
// WRAPACC-NEXT: nonnull 0
// WRAPACC-NEXT: pmf 0
// WRAPACC-NEXT: sarg_ref 0
// WRAPACC-NEXT: sret_ref 0
// WRAPACC-NEXT: scbind 0
// WRAPACC-NEXT: slambda 0
// WRAPACC-NEXT: tdtor 0
// WRAPACC-NEXT: bbind 0
// WRAPACC-NEXT: bcall 0
// WRAPACC-NEXT: nonnull_extra 0
