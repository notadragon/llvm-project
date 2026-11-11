// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a null pointer to data member applied with .*
// (ub:expr.mptr.oper.member.func.null.data) configured to "observe": the
// handler runs (kind=7, sem=2) and execution continues.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-data-observe.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
struct S {
  int a;
  int m;
};
int S::*volatile pmv = nullptr;
__attribute__((noinline)) int *addr(S &s, int S::*pm) { return &(s.*pm); }
int main() {
  S s{};
  int *p = addr(s, pmv);
  std::printf("RESULT=%d\n", p == (int *)((char *)&s - 1));
  return 0;
}
// CHECK: VIOL kind=7 sem=2 comment=[null pointer to data member]
// CHECK-NEXT: RESULT=1
