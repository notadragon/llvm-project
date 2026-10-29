// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose %libcxx_flags -o %t -Xclang -verify
// RUN: %t

// P4405 stage 2: folding never evaluates the predicate at run time and
// never proves one whose side effects it would have to drop: a predicate
// that is true whatever its operand's side effects is still not proven,
// and none of them run.  A proven predicate emits nothing either.  diagnose
// is used so that the program links and runs.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-not-evaluated.C
// (Clang also proves x > 0 || true, which has no side effects; the
// preconditions are analyzed at the call, D4405, where x == x folds with
// the argument)

#include <contracts>

static int evaluated, violations;
bool touch() { ++evaluated; return false; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x)
  pre(touch() || true)          // expected-note {{the predicate of the contract assertion}}
  pre(x == x)                   // expected-warning {{self-comparison always evaluates to true}}
  pre(x > 0 || true)            // proven
  post(r: (touch(), true))      // expected-warning {{contract assertion cannot be proven}}
{
  contract_assert(touch() && false); // expected-warning {{contract assertion cannot be proven}}
  contract_assert(false);       // expected-warning {{contract predicate is always false}}
  contract_assert(true);        // proven
  return x;
}

int main() {
  f(1); // expected-warning {{contract assertion cannot be proven}}
  if (evaluated != 0 || violations != 0)
    __builtin_abort();
}
