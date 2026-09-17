// Test multiple _Pre on one function: both fire independently.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  handler_called++;
}

int f(int x, int y) _Pre(x > 0) _Pre(y > 0) {
  return x + y;
}

int main(void) {
  // Both preconditions violated.
  f(-1, -1);
  if (handler_called != 2)
    __builtin_abort();

  return 0;
}
