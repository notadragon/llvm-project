// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:noexcept_observe \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: pointer arithmetic that wraps around the address space
// (ub:expr.add.out.of.bounds.pointer.arithmetic) configured to
// "noexcept_observe": the handler runs (kind=7, sem=6) and execution continues
// with the defined substitute.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-noexcept-observe.C)

#include <contracts>
#include <cstdio>
#include <cstdint>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
__attribute__((noinline)) char *add(char *p, long i) { return p + i; }
char *top = reinterpret_cast<char *>(UINTPTR_MAX - 1);
int main() {
  std::printf("RESULT=%d\n", (int)(add(top, 4) - top));
  return 0;
}
// CHECK: VIOL kind=7 sem=6 comment=[pointer arithmetic overflow]
// CHECK-NEXT: RESULT=0
