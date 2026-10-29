// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=diagnose -emit-llvm-only -verify %s

// D4405: -fcontract-analysis-semantic=diagnose gives the analysis evaluation
// of every contract assertion the diagnose semantic, leaving its runtime
// evaluations to -fcontract-evaluation-semantic (here ignore): a
// precondition, a postcondition and an assertion each get a
// -Wcontract-unproven warning.
//
// The analysis evaluation of a precondition is made at each call, in the
// caller, so the precondition is diagnosed at the call, with a note at its
// predicate; a postcondition's and an assertion's are made in the function
// body.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-diagnose.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-warning {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}

int use(int y) {
  return f(y); // expected-warning {{contract assertion cannot be proven}}
}
