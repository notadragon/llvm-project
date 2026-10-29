// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -fcontract-analysis-semantic=prove -emit-llvm-only -verify %s

// D4405, step 2 of "Adjusting to the Allowed Semantics": a prove or diagnose
// configured for a runtime evaluation moves to the analysis evaluation,
// which takes the stronger of it and its own configured semantic (ignore <
// diagnose < prove); the runtime evaluation becomes ignore.  Here the
// runtime evaluations are configured diagnose and the analysis evaluation
// prove, so each unproven assertion is an error, not a warning.
//
// The analysis evaluation of a precondition is made at each call, in the
// caller, so the precondition is diagnosed at the call, with a note at its
// predicate; a postcondition's and an assertion's are made in the function
// body.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-stronger.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-error {{contract assertion cannot be proven}}
  return x;
}

int use(int y) {
  return f(y); // expected-error {{contract assertion cannot be proven}}
}
