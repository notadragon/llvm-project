// Test _Pre with observe semantic: handler called, execution continues.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

#include <contracts.h>

static int handler_called = 0;
static int last_kind = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
  last_kind = stdc_contract_violation_kind(cv);
}

int guarded(int x) _Pre(x > 0) {
  return x;
}

int main(void) {
  // Good call: handler should not fire.
  guarded(1);
  if (handler_called != 0)
    __builtin_abort();

  // Bad call: handler should fire once with kind == PRE.
  guarded(-1);
  if (handler_called != 1)
    __builtin_abort();
  if (last_kind != STDC_CONTRACT_PRE)
    __builtin_abort();

  return 0;
}
