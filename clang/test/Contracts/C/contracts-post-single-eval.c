// A _Post(r: ...) must evaluate the return expression exactly once and check the
// value that is actually returned (D4299).
// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/contracts-post-single-eval.c.)

#include <contracts.h>

static int calls = 0;
static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  handler_called++;
}

static int side(void) { return ++calls; } // returns 1 on the first (only) call

int f(void) _Post(r: r > 0) { return side(); }

int main(void) {
  int v = f();
  if (calls != 1)          // evaluated exactly once
    __builtin_abort();
  if (v != 1)              // the (only) evaluation is what is returned
    __builtin_abort();
  if (handler_called != 0) // 1 > 0: postcondition holds
    __builtin_abort();
  return 0;
}
