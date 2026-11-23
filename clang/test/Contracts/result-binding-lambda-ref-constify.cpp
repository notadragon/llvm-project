// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// The result binding is const in a postcondition's predicate, including
// through a by-reference capture of a lambda there ([expr.prim.id.unqual]).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-binding-lambda-ref-constify.C

struct S {
  int b2() post(r: [&] { return ++r; }() > 0) { return 1; } // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
  int b3() post(r: [&r] { return ++r; }() > 0) { return 1; } // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
};
