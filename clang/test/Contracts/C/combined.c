// Test function with _Pre + _Post + _ContractAssert: all three fire.
// Pass 0 so pre(0 > 0) fires, assert(0 != 0) fires, return -0 == 0
// so post(r > 0) fires.  Verify 3 violations with correct kinds.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

#include <contracts.h>

static int handler_called = 0;
static int kinds[3];

void handle_contract_violation(const contract_violation_t *cv) {
  if (handler_called < 3)
    kinds[handler_called] = stdc_contract_violation_kind(cv);
  handler_called++;
}

int compute(int x) _Pre(x > 0) _Post(r: r > 0) {
  _ContractAssert(x != 0);
  return -x;  // Bug: returns negative, violating postcondition.
}

int main(void) {
  // x == 0: pre(0 > 0) fires, assert(0 != 0) fires,
  // return -0 == 0 so post(0 > 0) fires.
  compute(0);

  if (handler_called != 3)
    __builtin_abort();
  if (kinds[0] != STDC_CONTRACT_PRE)
    __builtin_abort();
  if (kinds[1] != STDC_CONTRACT_ASSERT)
    __builtin_abort();
  if (kinds[2] != STDC_CONTRACT_POST)
    __builtin_abort();

  return 0;
}
