// Test _ContractAssert with observe semantic: handler called, kind == ASSERT.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

#include <contracts.h>

static int handler_called = 0;
static int last_kind = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
  last_kind = stdc_contract_violation_kind(cv);
}

int main(void) {
  int x = -1;
  _ContractAssert(x > 0);

  if (handler_called != 1)
    __builtin_abort();
  if (last_kind != STDC_CONTRACT_ASSERT)
    __builtin_abort();

  return 0;
}
