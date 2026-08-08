// Test _Pre with ignore semantic: contract not evaluated, handler not called.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=ignore %libcxx_flags %s -o %t && %t

#include <contracts.h>

void handle_contract_violation(const contract_violation_t *cv) {
  // Handler must not be called under ignore semantic.
  __builtin_abort();
}

int guarded(int x) _Pre(x > 0) {
  return x;
}

int main(void) {
  // Bad value, but ignore semantic means no check occurs.
  guarded(-1);
  return 0;
}
