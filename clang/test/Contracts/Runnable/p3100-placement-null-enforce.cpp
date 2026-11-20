// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a null pointer passed to a non-allocating placement new
// (ub:expr.new.non.allocating.null) configured to "enforce": the handler runs
// (kind=7, sem=3), then the program terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-null-enforce.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
#include <new>
struct T {
  int x;
}; // trivial: `new (p) T` stores nothing
void *volatile nowhere = nullptr;
__attribute__((noinline)) T *make(void *p) { return new (p) T; }
int main() {
  T *t = make(nowhere);
  std::printf("RESULT=%d\n", t == nullptr);
  return 0;
}
// CHECK: VIOL kind=7 sem=3 comment=[null pointer passed to a non-allocating placement new]
// CHECK-NOT: RESULT
