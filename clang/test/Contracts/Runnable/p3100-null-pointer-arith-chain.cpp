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

// P3100: an indirection through a chain of pointer arithmetic on a null
// pointer, `*(p + 1 + 2)`, `(p + 1)[1]`, `p + k + 1`, is checked on the
// innermost pointer (DECISIONS.md E103 (a)), as `*(p + c)` is (E99 (a)): a
// scalar read or stored to and a member read or stored to are reported
// once under .access, a member's address once under .indirection.  So is
// one through pointer arithmetic with an offset that is not a constant,
// at every optimization level (E103 (a)), with or without a chain: then
// Clang's forms, `*(p + i)` read and stored to, a member read, stored to
// and its address, a reference bound, a class copied, a member function
// called and a pointer to member through `p + i`, `*(i + p)`, `*(p - i)`,
// `(p + 1)[i]`, `*(p + i + 1)`, `*(p + 1 + i)`, `*(p + i - 1)`, a
// reference bound to `*(p + 1 + i)` and a member function called through
// `p + 1 + i`.  Then gnu_gcc's further uses of a chain with constants: a
// reference bound, a class copied, a member function called, a pointer to
// member, a discarded indirection, a negative chain `*(p - 1 - 2)`, a
// member of an element of a sum and its address.  A non-null pointer is
// not reported.  With the pointer-arithmetic check configured too, under
// either leaf or both, each is reported as without it, and a negative
// offset also wraps around the address space and is reported once more by
// that check, for each sum that wraps.
// (CLANG-782: the chains with constants were checked on the inner sum and
// not reported; a non-constant offset was checked on `p + i`, reported at
// -O2 only, where the optimizer folds that check into one of `p`.)
// gnu_gcc checks `p + i` for a non-constant offset: the CLANG-745
// divergence, extended by E103 (a).
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3100-null-pointer-arith-chain*.C, whose header
// p3100-null-pointer-arith-chain.h has the same forms in this order.

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

struct S { int a, b; void f() {} };

#define NI __attribute__((noinline))
constexpr int k = 1;
NI int iread(int *ip) { return *(ip + 1 + 2); }
NI int isub(int *ip) { return (ip + 1)[0]; }
NI int iread_k(int *ip) { return *(ip + k + 1); }
NI void istore(int *ip) { *(ip + 1 + 2) = 1; }
NI void isubstore(int *ip) { (ip + 1)[1] = 1; }
NI int mread(S *sp) { return (sp + 1 + 2)->a; }
NI void mstore(S *sp) { (sp + 1 + 2)->a = 1; }
NI int msub(S *sp) { return (sp + 1)[1].a; }
NI int *maddr(S *sp) { return &(sp + 1 + 2)->a; }
// Clang's: an offset that is not a constant.
NI int inc(int *ip, int i) { return *(ip + i); }
NI void incst(int *ip, int i) { *(ip + i) = 1; }
NI int mnc(S *sp, int i) { return (sp + i)->a; }
NI void mncst(S *sp, int i) { (sp + i)->a = 1; }
NI int *maddrnc(S *sp, int i) { return &(sp + i)->a; }
NI void bindnc(S *sp, int i) { S &r = *(sp + i); (void)r; }
NI void copync(S *sp, int i) { S s2 = *(sp + i); (void)s2; }
NI void callnc(S *sp, int i) { (sp + i)->f(); }
NI int pmnc(S *sp, int i, int S::*p) { return (sp + i)->*p; }
NI int revnc(int *ip, int i) { return *(i + ip); }
NI int negnc(int *ip, int i) { return *(ip - i); }
NI int subnc(int *ip, int i) { return (ip + 1)[i]; }
NI int chainnc(int *ip, int i) { return *(ip + i + 1); }
NI int chainnc2(int *ip, int i) { return *(ip + 1 + i); }
NI int chainneg(int *ip, int i) { return *(ip + i - 1); }
NI void chainbind(S *sp, int i) { S &r = *(sp + 1 + i); (void)r; }
NI void chaincall(S *sp, int i) { (sp + 1 + i)->f(); }
// gnu_gcc's: more uses of a chain with constants.
NI void cbind(S *sp) { S &r = *(sp + 1 + 2); (void)r; }
NI void ccopy(S *sp) { S s2 = *(sp + 1 + 2); (void)s2; }
NI void ccall(S *sp) { (sp + 1 + 2)->f(); }
NI int cpm(S *sp, int S::*p) { return (sp + 1 + 2)->*p; }
NI void cdisc(S *sp) { (void)*(sp + 1 + 2); }
NI int cneg(int *ip) { return *(ip - 1 - 2); }
NI int csubm(S *sp) { return (sp + 1)[1].b; }
NI int *caddrsub(S *sp) { return &(sp + 1)[1].a; }

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
  int *ip = nv;
  S sa[4]{};
  int ia[4]{};
  volatile int one = 1, two = 2;
  COUNT("iread", iread(ip));
  COUNT("isub", isub(ip));
  COUNT("iread_k", iread_k(ip));
  COUNT("istore", istore(ip));
  COUNT("isubstore", isubstore(ip));
  COUNT("mread", mread(sp));
  COUNT("mstore", mstore(sp));
  COUNT("msub", msub(sp));
  COUNT("maddr", maddr(sp));
  COUNT("nonnull", iread(ia); isub(ia); iread_k(ia); istore(ia);
        isubstore(ia); mread(sa); mstore(sa); msub(sa); maddr(sa));
  COUNT("inc", inc(ip, one));
  COUNT("incst", incst(ip, one));
  COUNT("mnc", mnc(sp, one));
  COUNT("mncst", mncst(sp, one));
  COUNT("maddrnc", maddrnc(sp, one));
  COUNT("bindnc", bindnc(sp, one));
  COUNT("copync", copync(sp, one));
  COUNT("callnc", callnc(sp, one));
  COUNT("pmnc", pmnc(sp, one, &S::b));
  COUNT("revnc", revnc(ip, one));
  COUNT("negnc", negnc(ip, one));
  COUNT("subnc", subnc(ip, one));
  COUNT("chainnc", chainnc(ip, one));
  COUNT("chainnc2", chainnc2(ip, one));
  COUNT("chainneg", chainneg(ip, two));
  COUNT("chainbind", chainbind(sp, one));
  COUNT("chaincall", chaincall(sp, one));
  COUNT("nonnull_clang", inc(ia, one); incst(ia, one); mnc(sa, one);
        mncst(sa, one); maddrnc(sa, one); bindnc(sa, one); copync(sa, one);
        callnc(sa, one); pmnc(sa, one, &S::b); revnc(ia, one);
        negnc(ia + 1, one); subnc(ia, one); chainnc(ia, one);
        chainnc2(ia, one); chainneg(ia, two); chainbind(sa, one);
        chaincall(sa, one));
  COUNT("cbind", cbind(sp));
  COUNT("ccopy", ccopy(sp));
  COUNT("ccall", ccall(sp));
  COUNT("cpm", cpm(sp, &S::b));
  COUNT("cdisc", cdisc(sp));
  COUNT("cneg", cneg(ip));
  COUNT("csubm", csubm(sp));
  COUNT("caddrsub", caddrsub(sp));
  COUNT("nonnull_gcc", cbind(sa); ccopy(sa); ccall(sa); cpm(sa, &S::b);
        cdisc(sa); cneg(ia + 3); csubm(sa); caddrsub(sa));
  return 0;
}

// ALL: iread 1
// ALL-NEXT: isub 1
// ALL-NEXT: iread_k 1
// ALL-NEXT: istore 1
// ALL-NEXT: isubstore 1
// ALL-NEXT: mread 1
// ALL-NEXT: mstore 1
// ALL-NEXT: msub 1
// ALL-NEXT: maddr 1
// ALL-NEXT: nonnull 0
// ALL-NEXT: inc 1
// ALL-NEXT: incst 1
// ALL-NEXT: mnc 1
// ALL-NEXT: mncst 1
// ALL-NEXT: maddrnc 1
// ALL-NEXT: bindnc 1
// ALL-NEXT: copync 1
// ALL-NEXT: callnc 1
// ALL-NEXT: pmnc 1
// ALL-NEXT: revnc 1
// ALL-NEXT: negnc 1
// ALL-NEXT: subnc 1
// ALL-NEXT: chainnc 1
// ALL-NEXT: chainnc2 1
// ALL-NEXT: chainneg 1
// ALL-NEXT: chainbind 1
// ALL-NEXT: chaincall 1
// ALL-NEXT: nonnull_clang 0
// ALL-NEXT: cbind 1
// ALL-NEXT: ccopy 1
// ALL-NEXT: ccall 1
// ALL-NEXT: cpm 1
// ALL-NEXT: cdisc 1
// ALL-NEXT: cneg 1
// ALL-NEXT: csubm 1
// ALL-NEXT: caddrsub 1
// ALL-NEXT: nonnull_gcc 0

// IND: iread 0
// IND-NEXT: isub 0
// IND-NEXT: iread_k 0
// IND-NEXT: istore 0
// IND-NEXT: isubstore 0
// IND-NEXT: mread 0
// IND-NEXT: mstore 0
// IND-NEXT: msub 0
// IND-NEXT: maddr 1
// IND-NEXT: nonnull 0
// IND-NEXT: inc 0
// IND-NEXT: incst 0
// IND-NEXT: mnc 0
// IND-NEXT: mncst 0
// IND-NEXT: maddrnc 1
// IND-NEXT: bindnc 1
// IND-NEXT: copync 0
// IND-NEXT: callnc 1
// IND-NEXT: pmnc 0
// IND-NEXT: revnc 0
// IND-NEXT: negnc 0
// IND-NEXT: subnc 0
// IND-NEXT: chainnc 0
// IND-NEXT: chainnc2 0
// IND-NEXT: chainneg 0
// IND-NEXT: chainbind 1
// IND-NEXT: chaincall 1
// IND-NEXT: nonnull_clang 0
// IND-NEXT: cbind 1
// IND-NEXT: ccopy 0
// IND-NEXT: ccall 1
// IND-NEXT: cpm 0
// IND-NEXT: cdisc 1
// IND-NEXT: cneg 0
// IND-NEXT: csubm 0
// IND-NEXT: caddrsub 1
// IND-NEXT: nonnull_gcc 0

// ACC: iread 1
// ACC-NEXT: isub 1
// ACC-NEXT: iread_k 1
// ACC-NEXT: istore 1
// ACC-NEXT: isubstore 1
// ACC-NEXT: mread 1
// ACC-NEXT: mstore 1
// ACC-NEXT: msub 1
// ACC-NEXT: maddr 0
// ACC-NEXT: nonnull 0
// ACC-NEXT: inc 1
// ACC-NEXT: incst 1
// ACC-NEXT: mnc 1
// ACC-NEXT: mncst 1
// ACC-NEXT: maddrnc 0
// ACC-NEXT: bindnc 0
// ACC-NEXT: copync 1
// ACC-NEXT: callnc 0
// ACC-NEXT: pmnc 1
// ACC-NEXT: revnc 1
// ACC-NEXT: negnc 1
// ACC-NEXT: subnc 1
// ACC-NEXT: chainnc 1
// ACC-NEXT: chainnc2 1
// ACC-NEXT: chainneg 1
// ACC-NEXT: chainbind 0
// ACC-NEXT: chaincall 0
// ACC-NEXT: nonnull_clang 0
// ACC-NEXT: cbind 0
// ACC-NEXT: ccopy 1
// ACC-NEXT: ccall 0
// ACC-NEXT: cpm 1
// ACC-NEXT: cdisc 0
// ACC-NEXT: cneg 1
// ACC-NEXT: csubm 1
// ACC-NEXT: caddrsub 0
// ACC-NEXT: nonnull_gcc 0

// WRAP: iread 1
// WRAP-NEXT: isub 1
// WRAP-NEXT: iread_k 1
// WRAP-NEXT: istore 1
// WRAP-NEXT: isubstore 1
// WRAP-NEXT: mread 1
// WRAP-NEXT: mstore 1
// WRAP-NEXT: msub 1
// WRAP-NEXT: maddr 1
// WRAP-NEXT: nonnull 0
// WRAP-NEXT: inc 1
// WRAP-NEXT: incst 1
// WRAP-NEXT: mnc 1
// WRAP-NEXT: mncst 1
// WRAP-NEXT: maddrnc 1
// WRAP-NEXT: bindnc 1
// WRAP-NEXT: copync 1
// WRAP-NEXT: callnc 1
// WRAP-NEXT: pmnc 1
// WRAP-NEXT: revnc 1
// WRAP-NEXT: negnc 2
// WRAP-NEXT: subnc 1
// WRAP-NEXT: chainnc 1
// WRAP-NEXT: chainnc2 1
// WRAP-NEXT: chainneg 1
// WRAP-NEXT: chainbind 1
// WRAP-NEXT: chaincall 1
// WRAP-NEXT: nonnull_clang 0
// WRAP-NEXT: cbind 1
// WRAP-NEXT: ccopy 1
// WRAP-NEXT: ccall 1
// WRAP-NEXT: cpm 1
// WRAP-NEXT: cdisc 1
// WRAP-NEXT: cneg 3
// WRAP-NEXT: csubm 1
// WRAP-NEXT: caddrsub 1
// WRAP-NEXT: nonnull_gcc 0

// WRAPIND: iread 0
// WRAPIND-NEXT: isub 0
// WRAPIND-NEXT: iread_k 0
// WRAPIND-NEXT: istore 0
// WRAPIND-NEXT: isubstore 0
// WRAPIND-NEXT: mread 0
// WRAPIND-NEXT: mstore 0
// WRAPIND-NEXT: msub 0
// WRAPIND-NEXT: maddr 1
// WRAPIND-NEXT: nonnull 0
// WRAPIND-NEXT: inc 0
// WRAPIND-NEXT: incst 0
// WRAPIND-NEXT: mnc 0
// WRAPIND-NEXT: mncst 0
// WRAPIND-NEXT: maddrnc 1
// WRAPIND-NEXT: bindnc 1
// WRAPIND-NEXT: copync 0
// WRAPIND-NEXT: callnc 1
// WRAPIND-NEXT: pmnc 0
// WRAPIND-NEXT: revnc 0
// WRAPIND-NEXT: negnc 1
// WRAPIND-NEXT: subnc 0
// WRAPIND-NEXT: chainnc 0
// WRAPIND-NEXT: chainnc2 0
// WRAPIND-NEXT: chainneg 0
// WRAPIND-NEXT: chainbind 1
// WRAPIND-NEXT: chaincall 1
// WRAPIND-NEXT: nonnull_clang 0
// WRAPIND-NEXT: cbind 1
// WRAPIND-NEXT: ccopy 0
// WRAPIND-NEXT: ccall 1
// WRAPIND-NEXT: cpm 0
// WRAPIND-NEXT: cdisc 1
// WRAPIND-NEXT: cneg 2
// WRAPIND-NEXT: csubm 0
// WRAPIND-NEXT: caddrsub 1
// WRAPIND-NEXT: nonnull_gcc 0

// WRAPACC: iread 1
// WRAPACC-NEXT: isub 1
// WRAPACC-NEXT: iread_k 1
// WRAPACC-NEXT: istore 1
// WRAPACC-NEXT: isubstore 1
// WRAPACC-NEXT: mread 1
// WRAPACC-NEXT: mstore 1
// WRAPACC-NEXT: msub 1
// WRAPACC-NEXT: maddr 0
// WRAPACC-NEXT: nonnull 0
// WRAPACC-NEXT: inc 1
// WRAPACC-NEXT: incst 1
// WRAPACC-NEXT: mnc 1
// WRAPACC-NEXT: mncst 1
// WRAPACC-NEXT: maddrnc 0
// WRAPACC-NEXT: bindnc 0
// WRAPACC-NEXT: copync 1
// WRAPACC-NEXT: callnc 0
// WRAPACC-NEXT: pmnc 1
// WRAPACC-NEXT: revnc 1
// WRAPACC-NEXT: negnc 2
// WRAPACC-NEXT: subnc 1
// WRAPACC-NEXT: chainnc 1
// WRAPACC-NEXT: chainnc2 1
// WRAPACC-NEXT: chainneg 1
// WRAPACC-NEXT: chainbind 0
// WRAPACC-NEXT: chaincall 0
// WRAPACC-NEXT: nonnull_clang 0
// WRAPACC-NEXT: cbind 0
// WRAPACC-NEXT: ccopy 1
// WRAPACC-NEXT: ccall 0
// WRAPACC-NEXT: cpm 1
// WRAPACC-NEXT: cdisc 0
// WRAPACC-NEXT: cneg 3
// WRAPACC-NEXT: csubm 1
// WRAPACC-NEXT: caddrsub 0
// WRAPACC-NEXT: nonnull_gcc 0
