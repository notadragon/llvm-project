// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.new.non.allocating.null:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at a null pointer passed to a non-
// allocating placement new (ub:expr.new.non.allocating.null) under "enforce":
// the exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-null-throw.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
}
#include <new>
struct T {
  int x;
}; // trivial: `new (p) T` stores nothing
void *volatile nowhere = nullptr;
__attribute__((noinline)) T *make(void *p) { return new (p) T; }
int main() {
  try {
    T *t = make(nowhere);
    std::printf("RESULT=%d\n", t == nullptr);
  } catch (Boom &) {
    std::printf("CAUGHT\n");
    std::fflush(stdout);
  }
  std::printf("SURVIVED\n");
  return 0;
}
// CHECK: VIOL sem=3
// CHECK-NEXT: CAUGHT
// CHECK-NEXT: SURVIVED
