// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -Wall -Wextra -fsyntax-only -verify %s

// A parameter named in a precondition's predicate, or in a P3098 capture's
// initializer, is used there, so assigning it in the body does not make it
// "set but not used" (CLANG-633: Clang warned -Wunused-but-set-parameter,
// the contracts' scope having dropped the references it counted), on a
// function definition and on a member function defined in its class.  A
// parameter its contracts do not name still warns.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/param-set-used-in-contract.C.
// The EDG fork fixed the same (its EDG-74).

int f(int i) pre (i > 0) { i = 2; return 0; }                    // OK
int g(int i) post [j = i] (r: r != j) { i = 2; return 0; }       // OK
struct S {
  int m(int i) pre (i > 0) { i = 2; return 0; }                  // OK
};
int n(int i, int k) pre (k > 0) { i = 2; return k; } // expected-warning {{parameter 'i' set but not used}}
