// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A static_cast of an integral value outside the range of an unfixed
// enumeration is undefined ([expr.static.cast]/9), so it is not a core
// constant expression.  Clang rejects it; GCC accepts it (stock, gnu_gcc
// GCC-741, PR95701, deferred), and its watch test is the program below.  The
// floating form, which Clang accepts, is CLANG-718
// (OpenBugs/constexpr-enum-floating-cast.cpp).  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-enum-out-of-range.C

enum A { a0, a1, a2, a3 }; // values [0, 3]
constexpr A x = static_cast<A>(4); // expected-error {{constexpr variable 'x' must be initialized by a constant expression}} \
                                   // expected-note {{integer value 4 is outside the valid range of values [0, 3] for the enumeration type 'A'}}

// Control: an in-range value, integral and floating.
constexpr A y = static_cast<A>(3);
static_assert(y == a3);
constexpr A z = static_cast<A>(3.0);
static_assert(z == a3);
