// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mptr.oper.member.func.null.data:enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a null pointer to data member applied with .*
// (ub:expr.mptr.oper.member.func.null.data) configured to "enforce": the
// handler runs (kind=7, sem=3), then the program terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-mptr-null-data-enforce.C)

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
// CHECK: VIOL kind=7 sem=3 comment=[null pointer to data member]
// CHECK-NOT: RESULT
