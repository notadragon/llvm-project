// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-invalid-noreturn \
// RUN:   -fcontract-group-evaluation-semantic=ub:dcl.attr.noreturn.eventually.returns:enforce \
// RUN:   %libcxx_flags -o %t && not --crash %t 2>&1 | FileCheck %s
// P3100: a [[noreturn]] function that returns
// (ub:dcl.attr.noreturn.eventually.returns) configured to "enforce": the
// handler runs (kind=7, sem=3), then the program terminates.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noreturn-enforce.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
[[noreturn]] __attribute__((noinline)) void nr(int x) {
  if (x < 0)
    throw x;
}
void (*volatile fp)(int) = nr; // the caller does not know nr is noreturn
int main() {
  fp(1);
  std::printf("RESULT=1\n");
  return 0;
}
// CHECK: VIOL kind=7 sem=3 comment=[a {{\[\[}}noreturn{{\]\]}} function returned]
// CHECK-NOT: RESULT
