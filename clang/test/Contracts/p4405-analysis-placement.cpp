// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405 ("The Analysis Evaluation"): a precondition's analysis evaluation
// takes place at each call, in the caller, with the arguments substituted
// for the parameters -- never in the function itself, whose definition is
// not diagnosed, and not at all for a function nobody calls.  A
// postcondition's and a contract_assert's take place in the function body.
// Each is diagnosed whatever the semantic of the assertion's runtime
// evaluations, here enforce.  Optimized, so the proof shadow proves the
// guarded call.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-placement.C

int f(int x)
  pre(x > 0) // expected-note {{the predicate of the contract assertion}}
{
  return x;
}

int unused(int x)
  pre(x > 0)
{
  return x;
}

int g(int y) {
  int r = f(1);
  r += f(y); // expected-warning {{contract assertion cannot be proven}}
  if (y > 5)
    r += f(y);
  return r;
}

int h(int x)
  post(r: r > 0) // expected-warning {{contract assertion cannot be proven}}
{
  contract_assert(x != 3); // expected-warning {{contract assertion cannot be proven}}
  return x;
}

struct S {
  int v;
  void m(int x) pre(x > v); // expected-note {{the predicate of the contract assertion}}
};

void k(S &s, int x) {
  s.m(x); // expected-warning {{contract assertion cannot be proven}}
  if (x > s.v)
    s.m(x);
}
