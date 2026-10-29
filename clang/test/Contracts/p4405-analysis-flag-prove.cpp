// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=ignore -fcontract-analysis-semantic=prove -emit-llvm-only -verify %s

// D4405: -fcontract-analysis-semantic=prove gives the analysis evaluation of
// every contract assertion the prove semantic: each unproven one is an
// error.  The runtime evaluations are ignore (-fcontract-evaluation-semantic).
// A predicate that folds to true is proven.
//
// The analysis evaluation of a precondition is made at each call, in the
// caller, so the precondition is diagnosed at the call, with a note at its
// predicate; a postcondition's and an assertion's are made in the function
// body.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-prove.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-error {{contract assertion cannot be proven}}
  contract_assert(sizeof(int) > 0);
  return x;
}

int g(int y) {
  return f(y) + f(1); // expected-error {{contract assertion cannot be proven}}
}
