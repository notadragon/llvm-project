// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Subtracting pointers to two members of one object is undefined
// ([expr.add]/5.3: they are not elements of one array), so it is not a core
// constant expression.  Clang rejects it; GCC accepts it (stock, gnu_gcc
// GCC-740, PR126060, deferred), and its watch test is the program below.
// Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-sub-same-object.C

struct M { int a, b; };
constexpr long f() { M m{}; return &m.b - &m.a; } // expected-note {{subtracted pointers are not elements of the same array}}
static_assert(f() == 1); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
