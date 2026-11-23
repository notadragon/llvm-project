// A no-result _Post is evaluated at the return, AFTER the return value has been
// computed (P4299).
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-post-order.c.)

#include <contracts.h>

static int state = 0;
static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  handler_called++;
}

static int compute(void) {
  state = 1; // the return value computation establishes the postcondition
  return 0;
}

int h(void) _Post(state == 1) { return compute(); }

// Positive control: after compute() the state is 1, so this fails -- but it
// would hold if the check ran before the return value were computed, and it
// cannot fail if the postcondition is not evaluated at all.
int h2(void) _Post(state == 0) { return compute(); }

int main(void) {
  h();
  if (handler_called != 0) // state == 1 holds once compute() has run
    __builtin_abort();
  state = 0;
  h2();
  if (handler_called != 1)
    __builtin_abort();
  return 0;
}

// REQUIRES: contracts-libcxx, native
