// P3290 C trigger API: stdc_handle_quick_enforced_contract_violation terminates
// immediately, without invoking the handler.
// RUN: %clang -fcontracts-p4299 %libcxx_flags %s -o %t && not --crash %t
// (GCC mirror: gcc.dg/contracts/contracts-trigger-quick.c.)

#include <contracts.h>
#include <stdlib.h>

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  // The quick-enforced trigger must not call the handler; if it did and the
  // handler returned, the program would exit 0 and the test would fail.
  exit(0);
}

int main(void) {
  stdc_handle_quick_enforced_contract_violation("manual check failed");
  return 0;
}
