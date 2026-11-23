// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// `s.*pm` with a null pointer to data member is undefined
// ([expr.mptr.oper]/6), so it is not a core constant expression even when
// only its address is taken.  Clang rejects it; GCC accepts it (stock,
// gnu_gcc GCC-756, deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-null-data-member-pointer-address.C

struct S { int m = 1; };
constexpr bool f() {
  int S::*pm = nullptr;
  S s;
  int *p = &(s.*pm); // expected-note {{subexpression not valid in a constant expression}}
  return p != nullptr;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
