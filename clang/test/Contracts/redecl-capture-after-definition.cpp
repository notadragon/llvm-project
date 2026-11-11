// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// A capture-bearing postcondition may be repeated on a redeclaration that
// follows the definition.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-capture-after-definition.C

int k(const int x) post [c = x] (c > 0) { return x; }
int k(const int x) post [c = x] (c > 0);

int n(const int x) post [c = x] (c > 0);
int n(const int x) post [c = x] (c > 0);
int n(const int x) post [c = x] (c > 0) { return x; }

int m(const int x) post [c = x] (c > 0);
int m(const int y) post [c = y] (c > 0) { return y; }
