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
// RUN:   %libcxx_flags -o %t.ind0
// RUN: %t.ind0 | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.indirection:noexcept_observe \
// RUN:   %libcxx_flags -o %t.ind
// RUN: %t.ind | FileCheck %s --check-prefix=IND
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr.access:noexcept_observe \
// RUN:   %libcxx_flags -o %t.acc
// RUN: %t.acc | FileCheck %s --check-prefix=ACC
// P3100: a trivial copy or move of an empty class through a null pointer
// accesses no object -- the class has no member to read or write -- so the
// indirection is reported once under .indirection, not .access
// (CLANG-762, DECISIONS.md E86): assigning to `*ep` (`*ep = e`,
// `*ep = E()`, `ep->operator=(e)`), copying or moving from it (`e = *ep`,
// `E e2 = *ep`, `E e2 = std::move(*ep)`).  The same forms for a class with
// a member read or write it, the .access leaf.  A non-null pointer is not
// reported.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-empty-class-copy*.C, the
// same runs, whose header adds forms Clang tests in
// Runnable/p3100-null-empty-class-copy-extra.cpp.)

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

struct E {};
struct S { int m; };

#define NI __attribute__((noinline))
NI void eassign(E *ep, E &e) { *ep = e; }
NI void eassign_tmp(E *ep) { *ep = E(); }
NI void eassign_call(E *ep, E &e) { ep->operator=(e); }
NI void eassign_from(E *ep, E &e) { e = *ep; }
NI void ecopy(E *ep) { E e2 = *ep; (void)e2; }
NI void emove(E *ep) { E e2 = std::move(*ep); (void)e2; }
NI void sassign(S *sp, S &s) { *sp = s; }
NI void sassign_call(S *sp, S &s) { sp->operator=(s); }
NI void sassign_from(S *sp, S &s) { s = *sp; }
NI void scopy(S *sp) { S s2 = *sp; (void)s2; }

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
  E *ep = reinterpret_cast<E *>(vp);
  S *sp = reinterpret_cast<S *>(vp);
  E e;
  S s{};
  COUNT("eassign", eassign(ep, e));
  COUNT("eassign_tmp", eassign_tmp(ep));
  COUNT("eassign_call", eassign_call(ep, e));
  COUNT("eassign_from", eassign_from(ep, e));
  COUNT("ecopy", ecopy(ep));
  COUNT("emove", emove(ep));
  COUNT("sassign", sassign(sp, s));
  COUNT("sassign_call", sassign_call(sp, s));
  COUNT("sassign_from", sassign_from(sp, s));
  COUNT("scopy", scopy(sp));
  COUNT("nonnull", eassign(&e, e); eassign_call(&e, e); eassign_from(&e, e);
        ecopy(&e); emove(&e); sassign_call(&s, s); scopy(&s));
  return 0;
}

// ALL: eassign 1
// ALL-NEXT: eassign_tmp 1
// ALL-NEXT: eassign_call 1
// ALL-NEXT: eassign_from 1
// ALL-NEXT: ecopy 1
// ALL-NEXT: emove 1
// ALL-NEXT: sassign 1
// ALL-NEXT: sassign_call 1
// ALL-NEXT: sassign_from 1
// ALL-NEXT: scopy 1
// ALL-NEXT: nonnull 0

// IND: eassign 1
// IND-NEXT: eassign_tmp 1
// IND-NEXT: eassign_call 1
// IND-NEXT: eassign_from 1
// IND-NEXT: ecopy 1
// IND-NEXT: emove 1
// IND-NEXT: sassign 0
// IND-NEXT: sassign_call 0
// IND-NEXT: sassign_from 0
// IND-NEXT: scopy 0
// IND-NEXT: nonnull 0

// ACC: eassign 0
// ACC-NEXT: eassign_tmp 0
// ACC-NEXT: eassign_call 0
// ACC-NEXT: eassign_from 0
// ACC-NEXT: ecopy 0
// ACC-NEXT: emove 0
// ACC-NEXT: sassign 1
// ACC-NEXT: sassign_call 1
// ACC-NEXT: sassign_from 1
// ACC-NEXT: scopy 1
// ACC-NEXT: nonnull 0
