// Test _Pre with enforce semantic: handler called, then terminated.
// Handler calls _Exit(0) to signal success; reaching main's return
// means enforce failed to terminate.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=enforce %libcxx_flags %s -o %t && %t

#include <contracts.h>
#include <stdlib.h>

void handle_contract_violation(const contract_violation_t *cv) {
  // Handler is invoked; exit cleanly to signal success.
  _Exit(0);
}

int guarded(int x) _Pre(x > 0) {
  return x;
}

int main(void) {
  guarded(-1);
  // Should not reach here: enforce terminates after handler returns.
  return 1;
}
