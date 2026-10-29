// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   -fcontract-group-evaluation-semantic=ub:basic.align.object.alignment.nostore:enforce \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a throwing violation handler at a misaligned placement new that stores
// nothing (ub:basic.align.object.alignment.nostore) under "enforce": the
// exception propagates to the caller.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-align-throw.C)

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
alignas(int) char buf[4 * sizeof(int)];
void *volatile where = buf + 1;
__attribute__((noinline)) int *make(void *p) { return new (p) int; }
int main() {
  try {
    int *q = make(where);
    std::printf("RESULT=%d\n", (void *)q == where);
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
