// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A generic lambda's by-reference capture in a contract names the captured
// entity, which is const there ([expr.prim.id.unqual]), so modifying a
// parameter or the result name through it is ill-formed, as for a
// non-generic lambda.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/generic-lambda-modifies-param.C

void f(int i) pre([&](auto k) { return ++i > k; }(0)) {} // expected-error {{captured as const because it is inside a contract}}
void use() { f(1); }
int g() post(r: [&](auto k) { return ++r > k; }(0)) { return 1; } // expected-error {{cannot assign to variable 'r' because it is considered 'const' inside of a contract}}
