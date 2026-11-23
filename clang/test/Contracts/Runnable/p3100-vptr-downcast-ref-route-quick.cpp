// P3100 (UBSan runtime routing): -fsanitize-semantic=vptr:quick_enforce
// terminates on the reference form of a static_cast downcast
// (ub:expr.static.cast.base.class) without calling the handler and without
// any output.
// (GCC mirror: g++.dg/ubsan/p3100-vptr-downcast-ref-route-quick-enforce.C)

// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fsanitize=vptr -fsanitize-semantic=vptr:quick_enforce %libcxx_flags \
// RUN:   -o %t && not %t 2>&1 | FileCheck %s --allow-empty

#include <contracts>
#include <cstdio>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::printf("handler ran\n");
  std::fflush(stdout);
}

struct S { virtual ~S() {} int s = 1; };
struct T : S { int t = 2; };
struct U : S { int u = 3; };

__attribute__((noinline)) int lref(S &r) { return static_cast<T &>(r).s; }

int main() {
  U u;
  volatile int sink = lref(u);
  (void)sink;
  std::printf("survived\n");
  std::fflush(stdout);
  return 0;
}

// CHECK-NOT: handler ran
// CHECK-NOT: runtime error
// CHECK-NOT: survived
