// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3850 -fsyntax-only -verify %s

// Two friend declarations of the same function whose contracts disagree are
// diagnosed, as every other redeclaration is (CLANG-12: they were accepted
// silently; a friend's contracts are parsed at the end of the class, after
// its declaration was merged, and were never compared).  The fix is
// CLANG-639's (qualified-friend-member-contract-mismatch.cpp).  The
// namespace-scope control is contract-redecl-mismatch-namespace-control.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-friend-deferred-mismatch.C
// in the gnu_gcc fork, xfailed there (GCC-27, deferred: GCC discards the
// second friend declaration's deferred contracts before it could compare
// them).

struct C {
  friend int f(int x) pre(x > 0); // expected-note {{contract previously specified with a non-equivalent condition}}
  friend int f(int x) pre(x < 0); // expected-error {{function redeclaration differs in contract specifier sequence}} expected-note {{in contract specified here}}
  friend int g(int x) pre(x > 0);
  friend int g(int y) pre(y > 0); // OK, the same contract
};
