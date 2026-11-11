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

// P3100: the element at a subscript of a null pointer that is a constant
// variable, `p[k]`, is checked on the pointer itself, as at any subscript
// (DECISIONS.md E83 (c)): a scalar read or stored to, a member read or
// stored to, a class copied and a pointer to member's member read are
// reported once under .access; a member's or the element's address, a
// reference bound to either and a member function called once under
// .indirection; also with a `const` variable, `k[sp]` and `sp[k + 1]`.
// With the pointer-arithmetic check configured too, each is reported as
// without it.  A non-null pointer is not reported.  (Clang checks `p`
// for every subscript, constant or not; gnu_gcc's GCC-843 was a constant
// variable not read as a constant subscript.)
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/
// p3100-null-constant-subscript-variable*.C, whose header
// p3100-null-constant-subscript-variable.h has the same forms in this
// order.

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
const int kc = 1;
NI int iread(int *ip) { return ip[k]; }
NI void istore(int *ip) { ip[k] = 2; }
NI int mread(S *sp) { return sp[k].a; }
NI void mstore(S *sp) { sp[k].a = 1; }
NI int *maddr(S *sp) { return &sp[k].a; }
NI S *saddr(S *sp) { return &sp[k]; }
NI void mbind(S *sp) { int &r = sp[k].a; (void)r; }
NI void sbind(S *sp) { S &r = sp[k]; (void)r; }
NI void scopy(S *sp) { S s2 = sp[k]; (void)s2; }
NI void scall(S *sp) { sp[k].f(); }
NI int pm(S *sp, int S::*p) { return sp[k].*p; }
NI int iread_kc(int *ip) { return ip[kc]; }
NI int mread_kc(S *sp) { return sp[kc].a; }
NI int mread_rev(S *sp) { return k[sp].a; }
NI int mread_expr(S *sp) { return sp[k + 1].a; }

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
  S sa[3]{};
  int ia[3]{};
  COUNT("iread", iread(ip));
  COUNT("istore", istore(ip));
  COUNT("mread", mread(sp));
  COUNT("mstore", mstore(sp));
  COUNT("maddr", maddr(sp));
  COUNT("saddr", saddr(sp));
  COUNT("mbind", mbind(sp));
  COUNT("sbind", sbind(sp));
  COUNT("scopy", scopy(sp));
  COUNT("scall", scall(sp));
  COUNT("pm", pm(sp, &S::b));
  COUNT("iread_kc", iread_kc(ip));
  COUNT("mread_kc", mread_kc(sp));
  COUNT("mread_rev", mread_rev(sp));
  COUNT("mread_expr", mread_expr(sp));
  COUNT("nonnull", iread(ia); istore(ia); mread(sa); mstore(sa);
        maddr(sa); saddr(sa); mbind(sa); sbind(sa); scopy(sa); scall(sa);
        pm(sa, &S::b); iread_kc(ia); mread_kc(sa); mread_rev(sa);
        mread_expr(sa));
  return 0;
}

// ALL: iread 1
// ALL-NEXT: istore 1
// ALL-NEXT: mread 1
// ALL-NEXT: mstore 1
// ALL-NEXT: maddr 1
// ALL-NEXT: saddr 1
// ALL-NEXT: mbind 1
// ALL-NEXT: sbind 1
// ALL-NEXT: scopy 1
// ALL-NEXT: scall 1
// ALL-NEXT: pm 1
// ALL-NEXT: iread_kc 1
// ALL-NEXT: mread_kc 1
// ALL-NEXT: mread_rev 1
// ALL-NEXT: mread_expr 1
// ALL-NEXT: nonnull 0

// IND: iread 0
// IND-NEXT: istore 0
// IND-NEXT: mread 0
// IND-NEXT: mstore 0
// IND-NEXT: maddr 1
// IND-NEXT: saddr 1
// IND-NEXT: mbind 1
// IND-NEXT: sbind 1
// IND-NEXT: scopy 0
// IND-NEXT: scall 1
// IND-NEXT: pm 0
// IND-NEXT: iread_kc 0
// IND-NEXT: mread_kc 0
// IND-NEXT: mread_rev 0
// IND-NEXT: mread_expr 0
// IND-NEXT: nonnull 0

// ACC: iread 1
// ACC-NEXT: istore 1
// ACC-NEXT: mread 1
// ACC-NEXT: mstore 1
// ACC-NEXT: maddr 0
// ACC-NEXT: saddr 0
// ACC-NEXT: mbind 0
// ACC-NEXT: sbind 0
// ACC-NEXT: scopy 1
// ACC-NEXT: scall 0
// ACC-NEXT: pm 1
// ACC-NEXT: iread_kc 1
// ACC-NEXT: mread_kc 1
// ACC-NEXT: mread_rev 1
// ACC-NEXT: mread_expr 1
// ACC-NEXT: nonnull 0

// WRAP: iread 1
// WRAP-NEXT: istore 1
// WRAP-NEXT: mread 1
// WRAP-NEXT: mstore 1
// WRAP-NEXT: maddr 1
// WRAP-NEXT: saddr 1
// WRAP-NEXT: mbind 1
// WRAP-NEXT: sbind 1
// WRAP-NEXT: scopy 1
// WRAP-NEXT: scall 1
// WRAP-NEXT: pm 1
// WRAP-NEXT: iread_kc 1
// WRAP-NEXT: mread_kc 1
// WRAP-NEXT: mread_rev 1
// WRAP-NEXT: mread_expr 1
// WRAP-NEXT: nonnull 0
