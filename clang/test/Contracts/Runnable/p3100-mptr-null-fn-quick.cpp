// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:quick_enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a call through a null pointer to member function
// (ub:expr.mptr.oper.member.func.null.function) configured to "quick_enforce":
// terminates without calling the handler.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-fn-quick.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
struct S {
  int f() { return 1; }
  virtual int g() { return 2; }
};
int (S::*volatile pmv)() = nullptr;
__attribute__((noinline)) int call(S &s, int (S::*pm)()) { return (s.*pm)(); }
int main() {
  S s;
  std::printf("RESULT=%d\n", call(s, pmv));
  return 0;
}
// CHECK-NOT: VIOL
// CHECK-NOT: RESULT
