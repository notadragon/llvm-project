// P3595 location-based config selection for C: a rule keyed on the source file
// name selects the observe semantic for contracts in this file.  The default
// semantic is enforce (terminating); observe returns after the handler, so
// reaching the end of main proves the location rule matched.
// RUN: %clang -fcontracts-p4299 -fcontract-configuration-file=%S/contracts-config-location.json %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-config-location.c.)

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
  if (stdc_contract_violation_semantic(cv) != STDC_CONTRACT_OBSERVE)
    __builtin_abort();
}

int guarded(int x) _Pre(x > 0) { return x; }

int main(void) {
  guarded(-1);
  if (handler_called != 1)
    __builtin_abort();
  return 0;
}
