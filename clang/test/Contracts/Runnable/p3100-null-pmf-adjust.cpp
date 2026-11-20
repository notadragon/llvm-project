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

// P3100: a member function called through a pointer to member function on
// the object a null pointer points to, `(mp->*p)()`, `((*mp).*p)()`, is an
// indirection through the pointer, reported once under .indirection
// whatever the pointer to member function's `this` adjustment (here the
// offset of a second base, `&M::g2`), as with none (`&M::g1`); so is one
// on the element at a constant subscript of a null pointer, `(mp[1].*p)()`
// (DECISIONS.md E97 (c)), or at a constant offset from it,
// `((mp + 1)->*p)()` (DECISIONS.md E99 (a)).  A non-null pointer is not
// reported.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-null-pmf-adjust*.C
// (gnu_gcc GCC-835).

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

struct B1 { int q; void g1() {} };
struct B2 { int r; void g2() {} };
struct M : B1, B2 { int z; };

#define NI __attribute__((noinline))
NI void arrow(M *mp, void (M::*p)()) { (mp->*p)(); }
NI void star(M *mp, void (M::*p)()) { ((*mp).*p)(); }
NI void sub(M *mp, void (M::*p)()) { (mp[1].*p)(); }
NI void arith(M *mp, void (M::*p)()) { ((mp + 1)->*p)(); }

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
  M *mp = reinterpret_cast<M *>(nv);
  M ma[3]{};
  void (M::*adj)() = &M::g2;
  void (M::*noadj)() = &M::g1;
  COUNT("arrow", arrow(mp, adj));
  COUNT("star", star(mp, adj));
  COUNT("sub", sub(mp, adj));
  COUNT("arith", arith(mp, adj));
  COUNT("arrow0", arrow(mp, noadj));
  COUNT("star0", star(mp, noadj));
  COUNT("nonnull", arrow(ma, adj); star(ma, adj); sub(ma, adj);
        arith(ma, adj); arrow(ma, noadj); star(ma, noadj));
  return 0;
}

// ALL: arrow 1
// ALL-NEXT: star 1
// ALL-NEXT: sub 1
// ALL-NEXT: arith 1
// ALL-NEXT: arrow0 1
// ALL-NEXT: star0 1
// ALL-NEXT: nonnull 0

// IND: arrow 1
// IND-NEXT: star 1
// IND-NEXT: sub 1
// IND-NEXT: arith 1
// IND-NEXT: arrow0 1
// IND-NEXT: star0 1
// IND-NEXT: nonnull 0

// ACC: arrow 0
// ACC-NEXT: star 0
// ACC-NEXT: sub 0
// ACC-NEXT: arith 0
// ACC-NEXT: arrow0 0
// ACC-NEXT: star0 0
// ACC-NEXT: nonnull 0
