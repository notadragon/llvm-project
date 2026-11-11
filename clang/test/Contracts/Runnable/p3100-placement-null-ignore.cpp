// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:ignore \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a null pointer passed to a non-allocating placement new
// (ub:expr.new.non.allocating.null) configured to "ignore": no handler call,
// and execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-null-ignore.C)

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
// CHECK-NOT: VIOL
// CHECK: RESULT=1
