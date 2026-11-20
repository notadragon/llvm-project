// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A bound whose only non-constant part is a contract predicate is a constant
// bound, and the violation is diagnosed (after which Clang recovers by
// treating the bound as a VLA, hence the extension warning there).  A bound
// that is not constant apart from its predicate is a VLA, and its predicate
// is left to run time.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-contract-array-bound.C

bool rt();
// expected-note@-1 {{declared here}}

constexpr int f(const int &v)
  pre(false) // expected-error {{contract failed}}
{
  return 1;
}

int use(int p) {
  char a[f(p)]; // expected-warning {{variable length arrays in C++ are a Clang extension}}
  // expected-note@-1 {{in call to 'f(p)'}}
  return sizeof a;
}

constexpr int h(const int &v) pre(false) { return rt() ? 1 : 1; }
// expected-note@-1 {{non-constexpr function 'rt' cannot be used in a constant expression}}

int use_vla(int p) {
  char a[h(p)]; // expected-warning {{variable length arrays in C++ are a Clang extension}}
  // expected-note@-1 {{in call to 'h(p)'}}
  return sizeof a;
}
