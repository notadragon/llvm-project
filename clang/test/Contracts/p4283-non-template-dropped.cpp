// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=quick_enforce -fsyntax-only -verify %s

// P4283: a requires-clause on a contract assertion of a non-templated lambda
// or member function is rejected as on a non-templated function, and the
// rejected assertion is dropped: a postcondition naming a non-const value
// parameter draws no const-parameter error, and a constexpr function's false
// precondition is not checked in constant evaluation.  (From the EDG fork's
// p4283/requires_errors.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-non-template-dropped.C

template <class T> concept C = sizeof (T) <= 4;

auto lam = [] (int x) pre requires C<int> (x > 0) {};  // expected-error {{only allowed on templated functions}}
struct S {
  void m (int x) pre requires C<int> (x > 0) {}  // expected-error {{only allowed on templated functions}}
  void n (int x) { contract_assert requires C<int> (x > 0); }  // expected-error {{only allowed on templated functions}}
};

// Dropped: no error for the non-const parameter, and no check.
void d1 (int x) post requires (true) (x > 0);  // expected-error {{only allowed on templated functions}}
constexpr int d2 (int x) pre requires (true) (x > 0) { return x; }  // expected-error {{only allowed on templated functions}}
static_assert (d2 (-1) == -1);
