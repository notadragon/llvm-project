// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-718 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Clang only: GCC rejects it (though its
// diagnostic names neither the enumeration nor the range, gnu_gcc GCC-716).
//
// A static_cast of an out-of-range floating value to an unfixed enumeration
// is undefined ([expr.static.cast]/10: 4.0 converts to 4, outside the range
// [0, 3] of A), so it is not a constant expression; Clang accepts it.  The
// integral form and the in-range control are rejected and accepted in
// ../constexpr-enum-out-of-range.cpp.  When it is rejected, this XFAIL
// becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-enum-floating-cast-diagnostic.C)

enum A { a0, a1, a2, a3 }; // values [0, 3]
constexpr A x = static_cast<A>(4.0); // expected-error {{constexpr variable 'x' must be initialized by a constant expression}}
