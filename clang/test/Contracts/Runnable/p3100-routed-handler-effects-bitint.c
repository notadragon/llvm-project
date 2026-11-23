// RUN: %clang -std=c23 %s -fcontracts-p4299 -fcontracts-p3100 -fcontracts-p4298 -O2 \
// RUN:   -fsanitize=signed-integer-overflow \
// RUN:   -fsanitize-semantic=signed-integer-overflow:noexcept_observe \
// RUN:   %libcxx_flags -o %t
// RUN: %t | FileCheck %s

// P3100: with -fsanitize=signed-integer-overflow routed to the
// contract-violation handler with noexcept_observe, the code after a
// signed-overflow check of a _BitInt that reports and continues sees the
// handler's side effects, in the function holding the check and in its
// caller.  A _BitInt of more than 64 bits and one of fewer, an addition
// and a subtraction, in functions that are not inlined and in the caller,
// at -O2.
// Mirror: gcc/testsuite/gcc.dg/ubsan/p3100-routed-handler-effects-bitint.c
// (GCC-858).

#include <contracts.h>
#include <stdio.h>

static int reports;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  ++reports;
}

#define NI __attribute__((noinline))
NI _BitInt(127) addl(_BitInt(127) a, _BitInt(127) b) { return a + b; }
NI _BitInt(127) subl(_BitInt(127) a, _BitInt(127) b) { return a - b; }
NI _BitInt(31) adds(_BitInt(31) a, _BitInt(31) b) { return a + b; }
NI _BitInt(31) subs(_BitInt(31) a, _BitInt(31) b) { return a - b; }

#define COUNT(name, expr)                                                      \
  do {                                                                         \
    int before = reports;                                                      \
    expr;                                                                      \
    printf(name " %d\n", reports - before);                                    \
  } while (0)

int main(void) {
  volatile _BitInt(127) lmx = 85070591730234615865843651857942052863wb;
  volatile _BitInt(127) lmn = -lmx - 1, lone = 1;
  volatile _BitInt(31) smx = 1073741823wb, smn = -smx - 1, sone = 1;
  volatile _BitInt(127) l;
  volatile _BitInt(31) s;
  COUNT("addl", l = addl(lmx, lone));
  COUNT("subl", l = subl(lmn, lone));
  COUNT("adds", s = adds(smx, sone));
  COUNT("subs", s = subs(smn, sone));
  COUNT("iaddl", l = lmx + lone);
  COUNT("isubl", l = lmn - lone);
  COUNT("iadds", s = smx + sone);
  COUNT("isubs", s = smn - sone);
  (void)l;
  (void)s;
  return 0;
}

// CHECK: addl 1
// CHECK-NEXT: subl 1
// CHECK-NEXT: adds 1
// CHECK-NEXT: subs 1
// CHECK-NEXT: iaddl 1
// CHECK-NEXT: isubl 1
// CHECK-NEXT: iadds 1
// CHECK-NEXT: isubs 1
