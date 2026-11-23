// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// A redeclaration repeating a capturing postcondition token for token is
// accepted: the captures and the predicate are the same once the i-th
// capture of one declaration is matched with the i-th of the other
// ([dcl.contract.func], P3098).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-capturing-postcondition.C

int f(int x) post [x] (r : r == x + 1);
int f(int x) post [x] (r : r == x + 1) { return x + 1; }
