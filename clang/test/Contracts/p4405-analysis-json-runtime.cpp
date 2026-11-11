// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-analysis-json-runtime.json -fsyntax-only -verify %s

// D4405: the lookup of a runtime evaluation -- here the constant
// evaluation -- skips the entries with "analysis": true, so the first
// entry, which matches every analysis evaluation, does not make the
// precondition ignore: it is enforced in constant evaluation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-json-runtime.C

constexpr int f(int x) pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}

constexpr int good = f(1);
constexpr int bad = f(-1); // expected-error {{constexpr variable 'bad' must be initialized by a constant expression}}
  // expected-note@-1 {{in call to 'f(-1)'}}
