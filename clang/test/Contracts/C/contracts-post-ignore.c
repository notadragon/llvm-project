// Test _Post with ignore semantic: the postcondition is not evaluated (its
// predicate has a side effect that would show) and the handler is not called.
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=ignore %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-post-ignore.c.)

#include <contracts.h>

static int evaluated = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  __builtin_abort();
}

static int note(int r) {
  ++evaluated;
  return r > 0;
}

int positive(int x) _Post(r: note(r)) {
  return x;
}

int main(void) {
  positive(-1);
  if (evaluated != 0)
    __builtin_abort();
  return 0;
}

// REQUIRES: contracts-libcxx, native
