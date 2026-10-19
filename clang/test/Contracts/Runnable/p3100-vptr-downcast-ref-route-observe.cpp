// P3100 (UBSan runtime routing): the reference form of a static_cast downcast
// (ub:expr.static.cast.base.class, polymorphic) is checked by -fsanitize=vptr
// as the pointer form is, and its report is routed to the contract-violation
// handler.  vptr is recoverable, so with -fcontracts-p4298 it resolves to
// noexcept_observe: the handler runs (kind 7, semantic 6) and the program
// continues.  Casts to the object's own type, or to a base of it, do not
// report.
// (GCC mirror: g++.dg/ubsan/p3100-vptr-downcast-ref-route-observe.C)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=vptr %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>
#include <utility>

static int viols;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++viols;
  std::printf("handler kind=%d semantic=%d\n", (int)v.kind(),
              (int)v.semantic());
  std::fflush(stdout);
}

struct S { virtual ~S() {} int s = 1; };
struct T : S { int t = 2; };
struct T2 : T { int t2 = 3; };
struct U : S { int u = 4; };

__attribute__((noinline)) int lref(S &r) { return static_cast<T &>(r).s; }
__attribute__((noinline)) int xref(S &r) {
  return static_cast<T &&>(std::move(r)).s;
}

int main() {
  T t; T2 t2; U u;
  lref(t);
  lref(t2); // a T2 is a T
  xref(t);
  std::printf("clean=%d\n", viols);
  std::fflush(stdout);
  lref(u);
  xref(u);
  std::printf("survived=%d\n", viols);
  std::fflush(stdout);
  return 0;
}

// Nothing from UBSan itself: the handler lines are the only output.
// CHECK-NOT: runtime error
// CHECK: clean=0
// CHECK-NEXT: handler kind=7 semantic=6
// CHECK-NEXT: handler kind=7 semantic=6
// CHECK-NEXT: survived=2
