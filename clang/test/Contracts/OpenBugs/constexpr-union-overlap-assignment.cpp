// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-716 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-762).
//
// Assigning from an object that inexactly overlaps the target is undefined
// ([expr.assign]/8), here two members of a union, so f() is not a core
// constant expression; Clang accepts it.  When it is rejected, this XFAIL
// becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-union-overlap-assignment.C)

union U { int i; short s; };
constexpr int f() {
  U u;
  u.s = 1;
  u.i = u.s;
  return u.i;
}
static_assert(f() == 1); // expected-error {{not an integral constant expression}}
