// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-715 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-761).
//
// Naming a reference whose referent's storage has been deallocated is
// undefined ([basic.compound]/6, [expr.type]), so f() is not a core constant
// expression even though nothing is read; Clang accepts it.  When it is
// rejected, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-reference-to-freed-storage.C)

constexpr bool f() {
  int *ip = new int(1);
  int &i = *ip;
  delete ip;
  int &j = i;
  (void)j;
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}}
