// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-717 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-786).
//
// Binding a reference through a pointer to an automatic object whose scope
// has ended is an indirection through a pointer that points to no object,
// which is undefined ([expr.unary.op]/1, [basic.stc]/4), so t() is not a
// core constant expression; Clang accepts it.  Reading through the pointer
// is rejected (../constexpr-reference-through-invalid-pointer.cpp).  When it
// is rejected, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-reference-through-dangling-pointer.C)

constexpr bool t() {
  int *p;
  { int y = 1; p = &y; }
  int &r = *p;
  (void)r;
  return true;
}
static_assert(t()); // expected-error {{not an integral constant expression}}
