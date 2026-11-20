// RUN: %clang -std=c23 %s -fcontracts-p4299 -fcontracts-p3100 -fcontracts-p4298 \
// RUN:   -fsanitize=signed-integer-overflow \
// RUN:   -fsanitize-semantic=signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s
// RUN: %clang -std=c23 %s -fcontracts-p4299 -fcontracts-p3100 -fcontracts-p4298 -O2 \
// RUN:   -fsanitize=signed-integer-overflow \
// RUN:   -fsanitize-semantic=signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t2
// RUN: %t2 | FileCheck %s

// P3100: with -fsanitize=signed-integer-overflow routed to the
// contract-violation handler with noexcept_observe, a signed overflow of a
// _BitInt of more than 128 bits, an addition, and an overflowing
// multiplication of a _BitInt(127) are each reported once, at -O0 and -O2.
// Mirror: gcc/testsuite/gcc.dg/ubsan/open-bug-p3100-routed-bitint.c, the
// watch test of gnu_gcc GCC-867 (deferred, DECISIONS.md E122: there the
// first is not reported, libubsan calling it an unsigned overflow, and the
// second twice).

#include <contracts.h>
#include <stdio.h>

static int reports;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  ++reports;
}

__attribute__((noinline)) _BitInt(135) addh(_BitInt(135) a, _BitInt(135) b) {
  return a + b;
}

__attribute__((noinline)) _BitInt(127) mull(_BitInt(127) a, _BitInt(127) b) {
  return a * b;
}

int main(void) {
  volatile _BitInt(135) hmx = 21778071482940061661655974875633165533183wb;
  volatile _BitInt(135) hone = 1;
  volatile _BitInt(127) lmx = 85070591730234615865843651857942052863wb;
  int before = reports;
  volatile _BitInt(135) h = addh(hmx, hone);
  printf("addh %d\n", reports - before);
  before = reports;
  volatile _BitInt(127) l = mull(lmx, lmx);
  printf("mull %d\n", reports - before);
  (void)h;
  (void)l;
  return 0;
}

// CHECK: addh 1
// CHECK-NEXT: mull 1
