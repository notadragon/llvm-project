// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -fcontract-analysis-semantic=diagnose -emit-llvm-only -verify %s

// D4405, step 2: a configured runtime prove beats a configured analysis
// diagnose -- the analysis evaluation takes the stronger, prove.
//
// The analysis evaluation of a precondition is made at each call, in the
// caller, so the precondition is diagnosed at the call, with a note at its
// predicate; a postcondition's and an assertion's are made in the function
// body.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-stronger-2.C

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
