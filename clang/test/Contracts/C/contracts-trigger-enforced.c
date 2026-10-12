// P3290 C trigger API: stdc_handle_enforced_contract_violation invokes the
// handler and then terminates the program.
// RUN: %clang -fcontracts-p4299 %libcxx_flags %s -o %t && not --crash %t 2>&1 | FileCheck %s
// (GCC mirror: gcc.dg/contracts/contracts-trigger-enforced.c.)

#include <contracts.h>
#include <stdio.h>

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  // The user's handler runs (the default one would print something else),
  // then the enforced trigger terminates.
  fputs("user handler ran\n", stderr);
}

int main(void) {
  stdc_handle_enforced_contract_violation("manual check failed");
  fputs("returned from the trigger\n", stderr);
  return 0; // must not be reached
}

// CHECK: user handler ran
// CHECK-NOT: returned from the trigger

// REQUIRES: contracts-libcxx, native
