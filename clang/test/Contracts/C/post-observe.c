// Test _Post with observe semantic: handler called on violation, kind == POST.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

#include <contracts.h>

static int handler_called = 0;
static int last_kind = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
  last_kind = stdc_contract_violation_kind(cv);
}

int positive(int x) _Post(r: r > 0) {
  return x;
}

int main(void) {
  // Good call: postcondition satisfied.
  positive(1);
  if (handler_called != 0)
    __builtin_abort();

  // Bad call: returns -1, violating r > 0.
  positive(-1);
  if (handler_called != 1)
    __builtin_abort();
  if (last_kind != STDC_CONTRACT_POST)
    __builtin_abort();

  return 0;
}
