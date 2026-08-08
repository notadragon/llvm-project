// P3290 C trigger API: stdc_handle_observed_contract_violation invokes the
// handler with kind == MANUAL and the observe semantic, then returns normally.
// RUN: %clang -fcontracts-p4299 %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-trigger-observed.c.)

#include <contracts.h>

static int handler_called = 0;
static int last_kind = 0;
static int last_semantic = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
  last_kind = stdc_contract_violation_kind(cv);
  last_semantic = stdc_contract_violation_semantic(cv);
}

int main(void) {
  stdc_handle_observed_contract_violation("manual check failed");
  if (handler_called != 1)
    __builtin_abort();
  if (last_kind != STDC_CONTRACT_MANUAL)
    __builtin_abort();
  if (last_semantic != STDC_CONTRACT_OBSERVE)
    __builtin_abort();
  return 0;
}
