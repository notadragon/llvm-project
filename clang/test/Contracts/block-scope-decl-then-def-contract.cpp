// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A block-scope function declaration that is the first declaration carries a
// precondition, and the namespace-scope definition repeats it, which is
// valid.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/block-scope-decl-then-def-contract.C

void o(int t) {
  void loc(int x) pre(x > 0);
  loc(t);
}

void loc(int x) pre(x > 0) {}
