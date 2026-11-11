// Test _ContractAssert with enforce semantic: the handler is called with the
// assert kind and the enforce semantic, and the program is then terminated
// (the handler exits first, so reaching the end of main is the failure).
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=enforce %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-assert-enforce.c.)

#include <contracts.h>
#include <stdlib.h>

void handle_contract_violation(const contract_violation_t *cv) {
  if (stdc_contract_violation_kind(cv) != STDC_CONTRACT_ASSERT ||
      stdc_contract_violation_semantic(cv) != STDC_CONTRACT_ENFORCE ||
      !stdc_contract_violation_is_terminating(cv))
    abort();
  _Exit(0);
}

int main(void) {
  int x = -1;
  _ContractAssert(x > 0);
  return 1;
}

// REQUIRES: contracts-libcxx, native
