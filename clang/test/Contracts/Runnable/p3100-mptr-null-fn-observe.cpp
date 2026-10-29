// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.function:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a call through a null pointer to member function
// (ub:expr.mptr.oper.member.func.null.function) configured to "observe": the
// handler runs (kind=7, sem=2) and returns (here it longjmps out instead of
// letting the call go ahead).
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-fn-observe.C)

#include <contracts>
#include <csetjmp>
#include <cstdio>
namespace cs = std::contracts;
static std::jmp_buf env;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
  std::longjmp(env, 1); // do not continue into the undefined operation
}
struct S {
  int f() { return 1; }
  virtual int g() { return 2; }
};
int (S::*volatile pmv)() = nullptr;
__attribute__((noinline)) int call(S &s, int (S::*pm)()) { return (s.*pm)(); }
int main() {
  if (setjmp(env) == 0) {
    S s;
    std::printf("RESULT=%d\n", call(s, pmv));
  } else {
    std::printf("RESUMED\n");
  }
  return 0;
}
// CHECK: VIOL kind=7 sem=2 comment=[null pointer to member function]
// CHECK-NEXT: RESUMED
