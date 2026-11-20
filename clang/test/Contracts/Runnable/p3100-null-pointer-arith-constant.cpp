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

// P3100: an indirection through pointer arithmetic with a null pointer and
// a constant, `*(p + c)` (also `*(c + p)`, `*(p - c)`, a constant
// variable), is checked on the pointer itself, as `p[c]` is (DECISIONS.md
// E99 (a)): a scalar read or stored to, a member read or stored to
// (`(p + c)->a`, `(*(p + c)).a`, an element of an array member), a class
// copied from or stored to, the object of `(p + c)->*pm` and a member read
// through a vtable pointer, and a virtual member function called (which
// reads the vtable pointer, DECISIONS.md E102 (a)) are reported once under
// .access; the address of the element or of its member, a reference bound
// to either, a member function called on it, an empty class copied from or
// stored to it and a discarded `*(p + c)` access nothing and are reported
// once under .indirection.  A non-null pointer is
// not reported.  Then a trivial assignment called through it,
// `(p + c)->operator=(s)` (.access, a store), and, each .indirection, a
// trivial destructor, a static member function named through it, a call
// through a pointer to member function and a base's member function.
// Then gnu_gcc's extra forms: an element of an array member stored to
// (.access), an empty class assigned from and passed by value
// (.indirection); and the other spellings of a pointer plus or minus an
// integral constant expression (DECISIONS.md E105 (a)): `1 + p`, `p - 1`
// and `p + k` stored to, a member read, stored to and its address through
// each, a `const` variable as `c` (a scalar and a member read),
// `(sp - k)->a`, `(kc + sp)->a`, a reference bound to `*(1 + sp)`, a class
// copied from `*(sp - 1)`, a member function called through `sp + k` and
// the object of `(sp + kc)->*pm`.  With
// ub:expr.add.out.of.bounds.pointer.arithmetic configured too (the WRAP
// runs, and WRAPIND and WRAPACC with each leaf),
// every form is checked for null on the pointer as without it, once, under
// the leaf it has without it; `*(p - c)` also wraps around the address
// space, reported by that check too.  (CLANG-781: `vcall` was
// .indirection.)
// (CLANG-778: each form checked the address `p + c`, so none was
// reported.)
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3100-null-pointer-arith-constant*.C, whose header has the same forms
// in this order (its -wrap runs are the WRAP runs, its
// -wrap-{indirection,access} runs the WRAPIND and WRAPACC runs).

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
struct S { int a, b; int arr[2]; void f() {} static void sf() {} };
struct B { int x; void g() {} };
struct D : B { int y; };
struct V { virtual void h() {} int v; };

#define NI __attribute__((noinline))
constexpr int k = 1;
const int kc = 1;
NI void sbyval(S) {}
NI void ebyval(E) {}
NI int iread(int *ip) { return *(ip + 1); }
NI void istore(int *ip) { *(ip + 1) = 2; }
NI int iread_rev(int *ip) { return *(1 + ip); }
NI int iread_neg(int *ip) { return *(ip - 1); }
NI int iread_k(int *ip) { return *(ip + k); }
NI int mread(S *sp) { return (sp + 1)->a; }
NI void mstore(S *sp) { (sp + 1)->a = 1; }
NI int mread_dot(S *sp) { return (*(sp + 1)).a; }
NI int *maddr(S *sp) { return &(sp + 1)->a; }
NI void mbind(S *sp) { int &r = (sp + 1)->a; (void)r; }
NI void sbind(S *sp) { S &r = *(sp + 1); (void)r; }
NI S *saddr(S *sp) { return &*(sp + 1); }
NI void scopy(S *sp) { S s2 = *(sp + 1); (void)s2; }
NI void sfrom(S *sp, S &s) { s = *(sp + 1); }
NI void sstore(S *sp, S &s) { *(sp + 1) = s; }
NI void sarg(S *sp) { sbyval(*(sp + 1)); }
NI void scall(S *sp) { (sp + 1)->f(); }
NI int arrread(S *sp) { return (sp + 1)->arr[1]; }
NI int pm(S *sp, int S::*p) { return (sp + 1)->*p; }
NI void ecopy(E *ep) { E e2 = *(ep + 1); (void)e2; }
NI void estore(E *ep, E &e) { *(ep + 1) = e; }
NI void ebind(E *ep) { E &r = *(ep + 1); (void)r; }
NI void vcall(V *vp) { (vp + 1)->h(); }
NI int vread(V *vp) { return (vp + 1)->v; }
NI void disc(S *sp) { *(sp + 1); }
// Calls through the arithmetic.
NI void sassign(S *sp, S &s) { (sp + 1)->operator=(s); }
NI void sdtor(S *sp) { (sp + 1)->~S(); }
NI void sstatic(S *sp) { (sp + 1)->sf(); }
NI void pmf(S *sp, void (S::*p)()) { ((sp + 1)->*p)(); }
NI void bcall(D *dp) { (dp + 1)->g(); }
// GCC's: an array member element stored to, an empty class assigned from,
// passed by value.
NI void arrstore(S *sp) { (sp + 1)->arr[1] = 1; }
NI void efrom(E *ep, E &e) { e = *(ep + 1); }
NI void earg(E *ep) { ebyval(*(ep + 1)); }
// gnu_gcc's: the other spellings (DECISIONS.md E105 (a)), stored to, a
// member read, stored to and its address, a const variable, a reference
// bound, a class copied, a member function called and a pointer to member.
NI void istore_rev(int *ip) { *(1 + ip) = 2; }
NI void istore_neg(int *ip) { *(ip - 1) = 2; }
NI void istore_k(int *ip) { *(ip + k) = 2; }
NI int mread_rev(S *sp) { return (1 + sp)->a; }
NI int mread_neg(S *sp) { return (sp - 1)->a; }
NI int mread_k(S *sp) { return (sp + k)->a; }
NI void mstore_rev(S *sp) { (1 + sp)->a = 1; }
NI void mstore_neg(S *sp) { (sp - 1)->a = 1; }
NI void mstore_k(S *sp) { (sp + k)->a = 1; }
NI int *maddr_rev(S *sp) { return &(1 + sp)->a; }
NI int *maddr_neg(S *sp) { return &(sp - 1)->a; }
NI int *maddr_k(S *sp) { return &(sp + k)->a; }
NI int iread_kc(int *ip) { return *(ip + kc); }
NI int mread_kc(S *sp) { return (sp + kc)->a; }
NI int mread_negk(S *sp) { return (sp - k)->a; }
NI int mread_revk(S *sp) { return (kc + sp)->a; }
NI void sbind_rev(S *sp) { S &r = *(1 + sp); (void)r; }
NI void scopy_neg(S *sp) { S s2 = *(sp - 1); (void)s2; }
NI void scall_k(S *sp) { (sp + k)->f(); }
NI int pm_kc(S *sp, int S::*p) { return (sp + kc)->*p; }

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
  D *dp = reinterpret_cast<D *>(nv);
  int *ip = nv;
  S s{}, sa[3]{};
  D da[3]{};
  E e, ea[3];
  V va[3];
  int ia[3]{};
  COUNT("iread", iread(ip));
  COUNT("istore", istore(ip));
  COUNT("iread_rev", iread_rev(ip));
  COUNT("iread_neg", iread_neg(ip));
  COUNT("iread_k", iread_k(ip));
  COUNT("mread", mread(sp));
  COUNT("mstore", mstore(sp));
  COUNT("mread_dot", mread_dot(sp));
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
  COUNT("pm", pm(sp, &S::b));
  COUNT("ecopy", ecopy(ep));
  COUNT("estore", estore(ep, e));
  COUNT("ebind", ebind(ep));
  COUNT("vcall", vcall(vp));
  COUNT("vread", vread(vp));
  COUNT("disc", disc(sp));
  COUNT("nonnull", iread(ia); istore(ia); iread_rev(ia); iread_neg(ia + 1);
        iread_k(ia); mread(sa); mstore(sa); mread_dot(sa); maddr(sa);
        mbind(sa); sbind(sa); saddr(sa); scopy(sa); sfrom(sa, s);
        sstore(sa, s); sarg(sa); scall(sa); arrread(sa); pm(sa, &S::b);
        ecopy(ea); estore(ea, e); ebind(ea); vcall(va); vread(va);
        disc(sa));
  COUNT("sassign", sassign(sp, s));
  COUNT("sdtor", sdtor(sp));
  COUNT("sstatic", sstatic(sp));
  COUNT("pmf", pmf(sp, &S::f));
  COUNT("bcall", bcall(dp));
  COUNT("nonnull_extra", sassign(sa, s); sdtor(sa); sstatic(sa);
        pmf(sa, &S::f); bcall(da));
  COUNT("arrstore", arrstore(sp));
  COUNT("efrom", efrom(ep, e));
  COUNT("earg", earg(ep));
  COUNT("nonnull_gcc", arrstore(sa); efrom(ea, e); earg(ea));
  COUNT("istore_rev", istore_rev(ip));
  COUNT("istore_neg", istore_neg(ip));
  COUNT("istore_k", istore_k(ip));
  COUNT("mread_rev", mread_rev(sp));
  COUNT("mread_neg", mread_neg(sp));
  COUNT("mread_k", mread_k(sp));
  COUNT("mstore_rev", mstore_rev(sp));
  COUNT("mstore_neg", mstore_neg(sp));
  COUNT("mstore_k", mstore_k(sp));
  COUNT("maddr_rev", maddr_rev(sp));
  COUNT("maddr_neg", maddr_neg(sp));
  COUNT("maddr_k", maddr_k(sp));
  COUNT("iread_kc", iread_kc(ip));
  COUNT("mread_kc", mread_kc(sp));
  COUNT("mread_negk", mread_negk(sp));
  COUNT("mread_revk", mread_revk(sp));
  COUNT("sbind_rev", sbind_rev(sp));
  COUNT("scopy_neg", scopy_neg(sp));
  COUNT("scall_k", scall_k(sp));
  COUNT("pm_kc", pm_kc(sp, &S::b));
  COUNT("nonnull_e105", istore_rev(ia); istore_neg(ia + 1); istore_k(ia);
        mread_rev(sa); mread_neg(sa + 1); mread_k(sa); mstore_rev(sa);
        mstore_neg(sa + 1); mstore_k(sa); maddr_rev(sa); maddr_neg(sa + 1);
        maddr_k(sa); iread_kc(ia); mread_kc(sa); mread_negk(sa + 1);
        mread_revk(sa); sbind_rev(sa); scopy_neg(sa + 1); scall_k(sa);
        pm_kc(sa, &S::b));
  return 0;
}

// ALL: iread 1
// ALL-NEXT: istore 1
// ALL-NEXT: iread_rev 1
// ALL-NEXT: iread_neg 1
// ALL-NEXT: iread_k 1
// ALL-NEXT: mread 1
// ALL-NEXT: mstore 1
// ALL-NEXT: mread_dot 1
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
// ALL-NEXT: pm 1
// ALL-NEXT: ecopy 1
// ALL-NEXT: estore 1
// ALL-NEXT: ebind 1
// ALL-NEXT: vcall 1
// ALL-NEXT: vread 1
// ALL-NEXT: disc 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: sassign 1
// ALL-NEXT: sdtor 1
// ALL-NEXT: sstatic 1
// ALL-NEXT: pmf 1
// ALL-NEXT: bcall 1
// ALL-NEXT: nonnull_extra 0
// ALL-NEXT: arrstore 1
// ALL-NEXT: efrom 1
// ALL-NEXT: earg 1
// ALL-NEXT: nonnull_gcc 0
// ALL-NEXT: istore_rev 1
// ALL-NEXT: istore_neg 1
// ALL-NEXT: istore_k 1
// ALL-NEXT: mread_rev 1
// ALL-NEXT: mread_neg 1
// ALL-NEXT: mread_k 1
// ALL-NEXT: mstore_rev 1
// ALL-NEXT: mstore_neg 1
// ALL-NEXT: mstore_k 1
// ALL-NEXT: maddr_rev 1
// ALL-NEXT: maddr_neg 1
// ALL-NEXT: maddr_k 1
// ALL-NEXT: iread_kc 1
// ALL-NEXT: mread_kc 1
// ALL-NEXT: mread_negk 1
// ALL-NEXT: mread_revk 1
// ALL-NEXT: sbind_rev 1
// ALL-NEXT: scopy_neg 1
// ALL-NEXT: scall_k 1
// ALL-NEXT: pm_kc 1
// ALL-NEXT: nonnull_e105 0

// IND: iread 0
// IND-NEXT: istore 0
// IND-NEXT: iread_rev 0
// IND-NEXT: iread_neg 0
// IND-NEXT: iread_k 0
// IND-NEXT: mread 0
// IND-NEXT: mstore 0
// IND-NEXT: mread_dot 0
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
// IND-NEXT: pm 0
// IND-NEXT: ecopy 1
// IND-NEXT: estore 1
// IND-NEXT: ebind 1
// IND-NEXT: vcall 0
// IND-NEXT: vread 0
// IND-NEXT: disc 1
// IND-NEXT: nonnull 0
// IND-NEXT: sassign 0
// IND-NEXT: sdtor 1
// IND-NEXT: sstatic 1
// IND-NEXT: pmf 1
// IND-NEXT: bcall 1
// IND-NEXT: nonnull_extra 0
// IND-NEXT: arrstore 0
// IND-NEXT: efrom 1
// IND-NEXT: earg 1
// IND-NEXT: nonnull_gcc 0
// IND-NEXT: istore_rev 0
// IND-NEXT: istore_neg 0
// IND-NEXT: istore_k 0
// IND-NEXT: mread_rev 0
// IND-NEXT: mread_neg 0
// IND-NEXT: mread_k 0
// IND-NEXT: mstore_rev 0
// IND-NEXT: mstore_neg 0
// IND-NEXT: mstore_k 0
// IND-NEXT: maddr_rev 1
// IND-NEXT: maddr_neg 1
// IND-NEXT: maddr_k 1
// IND-NEXT: iread_kc 0
// IND-NEXT: mread_kc 0
// IND-NEXT: mread_negk 0
// IND-NEXT: mread_revk 0
// IND-NEXT: sbind_rev 1
// IND-NEXT: scopy_neg 0
// IND-NEXT: scall_k 1
// IND-NEXT: pm_kc 0
// IND-NEXT: nonnull_e105 0

// ACC: iread 1
// ACC-NEXT: istore 1
// ACC-NEXT: iread_rev 1
// ACC-NEXT: iread_neg 1
// ACC-NEXT: iread_k 1
// ACC-NEXT: mread 1
// ACC-NEXT: mstore 1
// ACC-NEXT: mread_dot 1
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
// ACC-NEXT: pm 1
// ACC-NEXT: ecopy 0
// ACC-NEXT: estore 0
// ACC-NEXT: ebind 0
// ACC-NEXT: vcall 1
// ACC-NEXT: vread 1
// ACC-NEXT: disc 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: sassign 1
// ACC-NEXT: sdtor 0
// ACC-NEXT: sstatic 0
// ACC-NEXT: pmf 0
// ACC-NEXT: bcall 0
// ACC-NEXT: nonnull_extra 0
// ACC-NEXT: arrstore 1
// ACC-NEXT: efrom 0
// ACC-NEXT: earg 0
// ACC-NEXT: nonnull_gcc 0
// ACC-NEXT: istore_rev 1
// ACC-NEXT: istore_neg 1
// ACC-NEXT: istore_k 1
// ACC-NEXT: mread_rev 1
// ACC-NEXT: mread_neg 1
// ACC-NEXT: mread_k 1
// ACC-NEXT: mstore_rev 1
// ACC-NEXT: mstore_neg 1
// ACC-NEXT: mstore_k 1
// ACC-NEXT: maddr_rev 0
// ACC-NEXT: maddr_neg 0
// ACC-NEXT: maddr_k 0
// ACC-NEXT: iread_kc 1
// ACC-NEXT: mread_kc 1
// ACC-NEXT: mread_negk 1
// ACC-NEXT: mread_revk 1
// ACC-NEXT: sbind_rev 0
// ACC-NEXT: scopy_neg 1
// ACC-NEXT: scall_k 0
// ACC-NEXT: pm_kc 1
// ACC-NEXT: nonnull_e105 0

// WRAP: iread 1
// WRAP-NEXT: istore 1
// WRAP-NEXT: iread_rev 1
// WRAP-NEXT: iread_neg 2
// WRAP-NEXT: iread_k 1
// WRAP-NEXT: mread 1
// WRAP-NEXT: mstore 1
// WRAP-NEXT: mread_dot 1
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
// WRAP-NEXT: pm 1
// WRAP-NEXT: ecopy 1
// WRAP-NEXT: estore 1
// WRAP-NEXT: ebind 1
// WRAP-NEXT: vcall 1
// WRAP-NEXT: vread 1
// WRAP-NEXT: disc 1
// WRAP-NEXT: nonnull 0
// WRAP-NEXT: sassign 1
// WRAP-NEXT: sdtor 1
// WRAP-NEXT: sstatic 1
// WRAP-NEXT: pmf 1
// WRAP-NEXT: bcall 1
// WRAP-NEXT: nonnull_extra 0
// WRAP-NEXT: arrstore 1
// WRAP-NEXT: efrom 1
// WRAP-NEXT: earg 1
// WRAP-NEXT: nonnull_gcc 0
// WRAP-NEXT: istore_rev 1
// WRAP-NEXT: istore_neg 2
// WRAP-NEXT: istore_k 1
// WRAP-NEXT: mread_rev 1
// WRAP-NEXT: mread_neg 2
// WRAP-NEXT: mread_k 1
// WRAP-NEXT: mstore_rev 1
// WRAP-NEXT: mstore_neg 2
// WRAP-NEXT: mstore_k 1
// WRAP-NEXT: maddr_rev 1
// WRAP-NEXT: maddr_neg 2
// WRAP-NEXT: maddr_k 1
// WRAP-NEXT: iread_kc 1
// WRAP-NEXT: mread_kc 1
// WRAP-NEXT: mread_negk 2
// WRAP-NEXT: mread_revk 1
// WRAP-NEXT: sbind_rev 1
// WRAP-NEXT: scopy_neg 2
// WRAP-NEXT: scall_k 1
// WRAP-NEXT: pm_kc 1
// WRAP-NEXT: nonnull_e105 0

// WRAPIND: iread 0
// WRAPIND-NEXT: istore 0
// WRAPIND-NEXT: iread_rev 0
// WRAPIND-NEXT: iread_neg 1
// WRAPIND-NEXT: iread_k 0
// WRAPIND-NEXT: mread 0
// WRAPIND-NEXT: mstore 0
// WRAPIND-NEXT: mread_dot 0
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
// WRAPIND-NEXT: pm 0
// WRAPIND-NEXT: ecopy 1
// WRAPIND-NEXT: estore 1
// WRAPIND-NEXT: ebind 1
// WRAPIND-NEXT: vcall 0
// WRAPIND-NEXT: vread 0
// WRAPIND-NEXT: disc 1
// WRAPIND-NEXT: nonnull 0
// WRAPIND-NEXT: sassign 0
// WRAPIND-NEXT: sdtor 1
// WRAPIND-NEXT: sstatic 1
// WRAPIND-NEXT: pmf 1
// WRAPIND-NEXT: bcall 1
// WRAPIND-NEXT: nonnull_extra 0
// WRAPIND-NEXT: arrstore 0
// WRAPIND-NEXT: efrom 1
// WRAPIND-NEXT: earg 1
// WRAPIND-NEXT: nonnull_gcc 0
// WRAPIND-NEXT: istore_rev 0
// WRAPIND-NEXT: istore_neg 1
// WRAPIND-NEXT: istore_k 0
// WRAPIND-NEXT: mread_rev 0
// WRAPIND-NEXT: mread_neg 1
// WRAPIND-NEXT: mread_k 0
// WRAPIND-NEXT: mstore_rev 0
// WRAPIND-NEXT: mstore_neg 1
// WRAPIND-NEXT: mstore_k 0
// WRAPIND-NEXT: maddr_rev 1
// WRAPIND-NEXT: maddr_neg 2
// WRAPIND-NEXT: maddr_k 1
// WRAPIND-NEXT: iread_kc 0
// WRAPIND-NEXT: mread_kc 0
// WRAPIND-NEXT: mread_negk 1
// WRAPIND-NEXT: mread_revk 0
// WRAPIND-NEXT: sbind_rev 1
// WRAPIND-NEXT: scopy_neg 1
// WRAPIND-NEXT: scall_k 1
// WRAPIND-NEXT: pm_kc 0
// WRAPIND-NEXT: nonnull_e105 0

// WRAPACC: iread 1
// WRAPACC-NEXT: istore 1
// WRAPACC-NEXT: iread_rev 1
// WRAPACC-NEXT: iread_neg 2
// WRAPACC-NEXT: iread_k 1
// WRAPACC-NEXT: mread 1
// WRAPACC-NEXT: mstore 1
// WRAPACC-NEXT: mread_dot 1
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
// WRAPACC-NEXT: pm 1
// WRAPACC-NEXT: ecopy 0
// WRAPACC-NEXT: estore 0
// WRAPACC-NEXT: ebind 0
// WRAPACC-NEXT: vcall 1
// WRAPACC-NEXT: vread 1
// WRAPACC-NEXT: disc 0
// WRAPACC-NEXT: nonnull 0
// WRAPACC-NEXT: sassign 1
// WRAPACC-NEXT: sdtor 0
// WRAPACC-NEXT: sstatic 0
// WRAPACC-NEXT: pmf 0
// WRAPACC-NEXT: bcall 0
// WRAPACC-NEXT: nonnull_extra 0
// WRAPACC-NEXT: arrstore 1
// WRAPACC-NEXT: efrom 0
// WRAPACC-NEXT: earg 0
// WRAPACC-NEXT: nonnull_gcc 0
// WRAPACC-NEXT: istore_rev 1
// WRAPACC-NEXT: istore_neg 2
// WRAPACC-NEXT: istore_k 1
// WRAPACC-NEXT: mread_rev 1
// WRAPACC-NEXT: mread_neg 2
// WRAPACC-NEXT: mread_k 1
// WRAPACC-NEXT: mstore_rev 1
// WRAPACC-NEXT: mstore_neg 2
// WRAPACC-NEXT: mstore_k 1
// WRAPACC-NEXT: maddr_rev 0
// WRAPACC-NEXT: maddr_neg 1
// WRAPACC-NEXT: maddr_k 0
// WRAPACC-NEXT: iread_kc 1
// WRAPACC-NEXT: mread_kc 1
// WRAPACC-NEXT: mread_negk 2
// WRAPACC-NEXT: mread_revk 1
// WRAPACC-NEXT: sbind_rev 0
// WRAPACC-NEXT: scopy_neg 2
// WRAPACC-NEXT: scall_k 0
// WRAPACC-NEXT: pm_kc 1
// WRAPACC-NEXT: nonnull_e105 0
