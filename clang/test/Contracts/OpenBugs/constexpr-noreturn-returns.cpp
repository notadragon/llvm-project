// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-711 in this fork's bug-reports/, llvm#211272; deferred by
// the user 2026-10-12, DECISIONS.md E73).  Clang only: GCC rejects it.
//
// A [[noreturn]] function that returns has undefined behavior
// ([dcl.attr.noreturn]/2), so f() is not a core constant expression and the
// static_assert must be an error; Clang only warns on the definition.  When
// it is rejected, this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/constexpr-noreturn-returns.C)

[[noreturn]] constexpr void g() {} // expected-warning {{function declared 'noreturn' should not return}}
constexpr bool f() { g(); return true; }
static_assert(f()); // expected-error {{not an integral constant expression}}
