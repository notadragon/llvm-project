// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at a call through a null pointer to
// member function (ub:expr.mptr.oper.member.func.null.function) under
// "enforce": the exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-fn-throw.C)

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
  int f() { return 1; }
  virtual int g() { return 2; }
};
int (S::*volatile pmv)() = nullptr;
__attribute__((noinline)) int call(S &s, int (S::*pm)()) { return (s.*pm)(); }
int main() {
  try {
    S s;
    std::printf("RESULT=%d\n", call(s, pmv));
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
