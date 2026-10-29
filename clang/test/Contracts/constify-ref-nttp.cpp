// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A template parameter of reference type names a const lvalue in a contract
// predicate ([expr.prim.id.unqual]), when it is named in the template and
// once its argument is substituted: ++R is rejected in a precondition and in
// a body contract_assert.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constify-binding-ref-nttp.C

template <int &R> void t1() pre(++R > 0) {}   // expected-error-re 1+ {{{{cannot assign to|read-only}}}}
template <int &R> void t2() {
  contract_assert(++R > 0);                   // expected-error-re 1+ {{{{cannot assign to|read-only}}}}
}

int g;
void use() {
  t1<g>();
  t2<g>();
}
