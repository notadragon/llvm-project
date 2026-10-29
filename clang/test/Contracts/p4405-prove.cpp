// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -mconstructor-aliases -emit-llvm-only -verify %s

// P4405: every evaluation with the prove semantic is an error until
// something can be proven -- a precondition, diagnosed at each call (D4405),
// a postcondition and an assertion, diagnosed in the function body, ahead
// of where their checks would be emitted.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-prove.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-error {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-error {{contract assertion cannot be proven}}
  return x;
}

void g(int x) {
  contract_assert(x > 0);       // expected-error {{contract assertion cannot be proven}}
  f(x);                         // expected-error {{contract assertion cannot be proven}}
}

// A destructor's contract assertion is diagnosed once, not once per variant
// (with and without constructor aliases), and a constructor's precondition
// once per call.  A virtual base makes the complete and base variants two
// bodies.
struct V {};
struct S : virtual V {
  int v;
  S(int x)
    pre(x > 0)                  // expected-note {{the predicate of the contract assertion}}
    : v(x) {}
  ~S() { contract_assert(v > 0); } // expected-error {{contract assertion cannot be proven}}
};

S s(1);

void make(int y) {
  S t(y);                       // expected-error {{contract assertion cannot be proven}}
}
