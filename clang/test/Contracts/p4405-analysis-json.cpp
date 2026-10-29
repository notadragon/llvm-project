// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-analysis-json.json -emit-llvm-only -verify %s

// D4405 ("P3595R1 Analysis Evaluation Configuration"): entries with
// "analysis": true select the semantic of the analysis evaluation in a
// lookup of their own through the same list.  Only they match that lookup
// -- the first entry (no "analysis", kind pre, ignore) does not stop the
// precondition's analysis from being prove -- and the runtime lookups skip
// them.  An earlier entry excludes a namespace; "analysis" with
// "constexpr": true never matches (else every assertion would be proven),
// and with "constexpr": false it does; "analysis": false is the same as
// leaving the key out.  The runtime evaluations are ignore throughout.
//
// The analysis evaluation of a precondition is made at each call, in the
// caller, so the precondition is diagnosed at the call, with a note at its
// predicate; a postcondition's and an assertion's are made in the function
// body.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-json.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}

int use(int y) {
  return f(y); // expected-error {{contract assertion cannot be proven}}
}

namespace thirdparty {
int g(int x)
  pre(x > 0)
  post(r: r > 0)
{
  contract_assert(x != 3);
  return x;
}
} // namespace thirdparty

int use_thirdparty(int y) {
  return thirdparty::g(y);
}
