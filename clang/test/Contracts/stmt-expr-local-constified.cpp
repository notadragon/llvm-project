// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A variable declared inside a GNU statement expression in a contract
// predicate is declared inside the contract, so it is not const
// ([expr.prim.id.unqual]); one declared outside still is, named from inside
// the statement expression.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-constified.C

void f() { contract_assert(__extension__({ int i = 0; ++i; }) > 0); }

template <class T> void t() { contract_assert(__extension__({ T i = 0; ++i; }) > 0); }
template void t<int>();

void g() {
  int j = 0;
  contract_assert(__extension__({ ++j; }) > 0); // expected-error {{inside of a contract}}
}
