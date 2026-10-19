// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 \
// RUN:   %libcxx_flags -o %t && %t 2>&1 | FileCheck %s
// P3100: a misaligned placement new that stores nothing
// (ub:basic.align.object.alignment.nostore) left at the default "assume": no
// check is emitted.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-placement-align-assume.C)

#include <contracts>
#include <cstdio>
namespace cs = std::contracts;
void handle_contract_violation(const cs::contract_violation &v) {
  std::printf("VIOL kind=%d sem=%d comment=[%s]\n", (int)v.kind(),
              (int)v.semantic(), v.comment());
  std::fflush(stdout);
}
#include <new>
alignas(int) char buf[4 * sizeof(int)];
void *volatile where = buf + 1;
__attribute__((noinline)) int *make(void *p) { return new (p) int; }
int main() {
  int *q = make(where);
  std::printf("RESULT=%d\n", (void *)q == where);
  return 0;
}
// CHECK-NOT: VIOL
// CHECK: RESULT=1
