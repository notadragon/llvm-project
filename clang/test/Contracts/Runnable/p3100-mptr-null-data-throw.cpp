// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at a null pointer to data member applied
// with .* (ub:expr.mptr.oper.member.func.null.data) under "enforce": the
// exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-data-throw.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
}
struct S {
  int a;
  int m;
};
int S::*volatile pmv = nullptr;
__attribute__((noinline)) int *addr(S &s, int S::*pm) { return &(s.*pm); }
int main() {
  try {
    S s{};
    int *p = addr(s, pmv);
    std::printf("RESULT=%d\n", p == (int *)((char *)&s - 1));
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
