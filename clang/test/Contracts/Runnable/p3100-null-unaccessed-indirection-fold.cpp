// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t | FileCheck %s
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -w -O2 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.unary.dereference.nullptr:noexcept_observe \
// RUN:   %libcxx_flags -o %t2 && %t2 | FileCheck %s
// P3100: a constexpr call that is not manifestly constant-evaluated, and
// that Clang would fold (an `if` condition), is not folded when it performs
// an indirection through a null pointer, so the run-time check stays and
// reports once (CLANG-753): the unaccessed indirections (`sp->sf()`,
// `(*sp).sf()`, `sp->ck`, `sp->E`, `&sp->k`, `&*p`, `&p[0]`, `*p;`,
// `&*fp`, `*fp;`) and the checks that already existed for an indirection
// that accesses nothing (`&sp->m`, `&sp->arr[1]`, `&pp[0][1]`, a reference
// bound to `*p` or `*fp`), each of which the fold used to drop.  Manifestly
// constant evaluation is unchanged: the static_asserts with a pointer to an
// object hold, and a constexpr variable is constant-initialized with no
// report.  `__builtin_constant_p` of such a call is 0.
// (GCC mirror: the sfc_constexpr form of
// g++.dg/contracts/cpp26/p3100-null-unaccessed-indirection.C; GCC's
// speculative evaluation gives up on a P3100 null check of a null pointer.)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;

static int reports;
void handle_contract_violation(const cs::contract_violation &) { ++reports; }

struct S {
  static int k;
  static constexpr int sf() { return 3; }
  static constexpr int ck = 6;
  enum { E = 5 };
  int m;
  int arr[2];
};
int S::k = 4;
using F = int();
constexpr int seven() { return 7; }

constexpr int sfc(S *sp) { return sp->sf(); }
constexpr int sfc_star(S *sp) { return (*sp).sf(); }
constexpr int sck(S *sp) { return sp->ck; }
constexpr int se(S *sp) { return sp->E; }
constexpr bool skaddr(S *sp) { return &sp->k != nullptr; }
constexpr bool addr(int *p) { return &*p == nullptr; }
constexpr bool addr0(int *p) { return &p[0] == nullptr; }
constexpr int disc(int *p) { *p; return 1; }
constexpr bool faddr(F *fp) { return &*fp == nullptr; }
constexpr int fdisc(F *fp) { *fp; return 1; }
constexpr bool maddr(S *sp) { return &sp->m != nullptr; }
constexpr bool aaddr(S *sp) { return &sp->arr[1] != nullptr; }
constexpr bool paddr(int (*pp)[2]) { return &pp[0][1] != nullptr; }
constexpr int obind(int *p) { int &r = *p; (void)r; return 1; }
constexpr int fbind(F *fp) { F &r = *fp; (void)r; return 1; }

constexpr S gs{};
constexpr int gi[2][2]{};
static_assert(sfc(const_cast<S *>(&gs)) == 3);
static_assert(sfc_star(const_cast<S *>(&gs)) == 3);
static_assert(sck(const_cast<S *>(&gs)) == 6);
static_assert(se(const_cast<S *>(&gs)) == 5);
static_assert(!addr(const_cast<int *>(&gi[0][0])));
static_assert(maddr(const_cast<S *>(&gs)));
static_assert(paddr(const_cast<int (*)[2]>(gi)));
static_assert(fdisc(seven) == 1 && !faddr(seven) && fbind(seven) == 1);
constexpr int cv = sfc(const_cast<S *>(&gs)) + sck(const_cast<S *>(&gs));

static volatile bool sink;
#define COUNT(name, cond)                                                      \
  do {                                                                         \
    int before = reports;                                                      \
    if (cond)                                                                  \
      sink = true;                                                             \
    std::printf(name " %d\n", reports - before);                               \
  } while (0)

int main() {
  COUNT("sfc", sfc(nullptr));
  COUNT("sfc_star", sfc_star(nullptr));
  COUNT("sck", sck(nullptr));
  COUNT("se", se(nullptr));
  COUNT("skaddr", skaddr(nullptr));
  COUNT("addr", addr(nullptr));
  COUNT("addr0", addr0(nullptr));
  COUNT("disc", disc(nullptr));
  COUNT("faddr", faddr(nullptr));
  COUNT("fdisc", fdisc(nullptr));
  COUNT("maddr", maddr(nullptr));
  COUNT("aaddr", aaddr(nullptr));
  COUNT("paddr", paddr(nullptr));
  COUNT("obind", obind(nullptr));
  COUNT("fbind", fbind(nullptr));
  std::printf("bcp %d\n", __builtin_constant_p(sfc(nullptr)));
  int before = reports;
  if (cv != 9 || sfc(const_cast<S *>(&gs)) != 3 || !maddr(const_cast<S *>(&gs)))
    return 1;
  std::printf("nonnull %d\n", reports - before);
  return 0;
}

// CHECK: sfc 1
// CHECK-NEXT: sfc_star 1
// CHECK-NEXT: sck 1
// CHECK-NEXT: se 1
// CHECK-NEXT: skaddr 1
// CHECK-NEXT: addr 1
// CHECK-NEXT: addr0 1
// CHECK-NEXT: disc 1
// CHECK-NEXT: faddr 1
// CHECK-NEXT: fdisc 1
// CHECK-NEXT: maddr 1
// CHECK-NEXT: aaddr 1
// CHECK-NEXT: paddr 1
// CHECK-NEXT: obind 1
// CHECK-NEXT: fbind 1
// CHECK-NEXT: bcp 0
// CHECK-NEXT: nonnull 0
