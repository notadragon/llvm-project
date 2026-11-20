// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at pointer arithmetic that wraps around
// the address space (ub:expr.add.out.of.bounds.pointer.arithmetic) under
// "enforce": the site is in a function that may throw, so the exception
// propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-throw.C)

#include <contracts>
#include <cstdio>
#include <cstdint>
namespace cs = std::contracts;
struct Boom {};
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL sem=%d\n", (int)v.semantic());
  std::fflush(stdout);
  throw Boom{};
}
__attribute__((noinline)) char *add(char *p, long i) { return p + i; }
char *top = reinterpret_cast<char *>(UINTPTR_MAX - 1);
int main() {
  try {
    std::printf("RESULT=%d\n", (int)(add(top, 4) - top));
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
