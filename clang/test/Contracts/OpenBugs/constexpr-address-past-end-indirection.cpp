// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-712 in this fork's bug-reports/; llvm#65119 closed
// not_planned; deferred by the user 2026-10-12, DECISIONS.md E73).  Stock
// Clang and GCC alike (gnu_gcc GCC-785).
//
// `*(a + 1)` on a one-element array is an indirection through a pointer that
// points to no object, which is undefined ([expr.unary.op]/1) even when only
// its address is taken, so f() is not a core constant expression; Clang
// accepts `&*(a + 1)`.  Binding a reference to it, and reading through it,
// are rejected (../constexpr-reference-through-invalid-pointer.cpp).  When it
// is rejected, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-address-past-end-indirection.C)

constexpr bool f() {
  int a[1] = {0};
  int *p = &*(a + 1);
  return p != nullptr;
}
static_assert(f()); // expected-error {{not an integral constant expression}}
