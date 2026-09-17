// A contract written on a parameter's own function declarator has nothing
// to attach to, so it must be diagnosed and dropped.  Left unconsumed it
// reaches Declarator::clear() and trips its "Late-parsed contracts
// unhandled" assertion -- a compiler crash on one line of C.
//
// The other way to get this wrong is to mis-attach the parameter's contract
// to the *enclosing* function, so that `_Pre (0)` fires against a function
// that has no precondition at all.  Both compilers ignore the contract and
// say so.

// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe -Xclang -verify %libcxx_flags %s -o %t && %t

#include <contracts.h>

static int handler_called = 0;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  handler_called++;
}

// The enclosing function has no contract; the parameter's must not fire.
int no_contract_of_its_own(void (*cb)(int) _Pre(0), int v) { // expected-warning {{contract on a parameter declarator is ignored}}
  (void)cb;
  return v;
}

// The enclosing function has its own contract; only that one may fire.
int has_own_contract(void (*cb)(int) _Pre(0), int v) _Pre(v > 0) { // expected-warning {{contract on a parameter declarator is ignored}}
  (void)cb;
  return v;
}

int main(void) {
  handler_called = 0;
  no_contract_of_its_own(0, 1);
  if (handler_called != 0)
    __builtin_abort();

  handler_called = 0;
  has_own_contract(0, 1);
  if (handler_called != 0)
    __builtin_abort();

  handler_called = 0;
  has_own_contract(0, -1);
  if (handler_called != 1)
    __builtin_abort();

  return 0;
}
