// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405 ("The Analysis Evaluation"): a capture-less lambda converted to a
// pointer to function points to a static invoker that calls the function
// call operator with its own (unknown) arguments; that call is part of the
// indirect call and has no analysis evaluation -- for a generic lambda too,
// whose invoker is instantiated on conversion, and whether or not the
// operator was called directly before.  The preconditions here are
// analyzed only because the configuration says so, so the conversion is
// not diagnosed either ("Taking the Address of a Function"), and the
// direct calls are analyzed.  Unoptimized, so every analysis evaluation is
// decided at once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-lambda-thunk.C

int use(int y) {
  auto g = [](auto x) pre(x > 0) { return x; }; // expected-note {{the predicate of the contract assertion}}
  int r = g(1);
  int (*p)(int) = g;
  long (*pl)(long) = g;
  auto n = [](int x) pre(x > 0) { return x; }; // expected-note {{the predicate of the contract assertion}}
  int (*q)(int) = n;
  r += n(y);         // expected-warning {{contract assertion cannot be proven}}
  r += g(y);         // expected-warning {{contract assertion cannot be proven}}
  return r + p(y) + (int)pl(y) + q(y);
}
