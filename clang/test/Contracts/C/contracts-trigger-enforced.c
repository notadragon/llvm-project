// P3290 C trigger API: stdc_handle_enforced_contract_violation invokes the
// handler and then terminates the program.
// RUN: %clang -fcontracts-p4299 %libcxx_flags %s -o %t && not --crash %t
// (GCC mirror: gcc.dg/contracts/contracts-trigger-enforced.c.)

#include <contracts.h>

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  // Handler runs, then the enforced trigger terminates.
}

int main(void) {
  stdc_handle_enforced_contract_violation("manual check failed");
  return 0; // must not be reached
}
