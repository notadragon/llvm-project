// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A by-reference lambda capture in a predicate names the captured entity, so
// it is const there ([expr.prim.id.unqual]): the lambda cannot modify a
// parameter or the result name, in a free function's contract as in a member
// function's or in a contract_assert.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-modifies-param-in-pre.C

void f(int i) pre([&] { return ++i > 0; }()) {}                 // expected-error {{inside a contract}}

void f2(int i) { contract_assert([&] { return ++i > 0; }()); }  // expected-error {{inside a contract}}

struct S { void m(int i) pre([&] { return ++i > 0; }()); };      // expected-error {{inside a contract}}

int g() post(r: [&] { return ++r > 0; }()) { return 1; }            // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
