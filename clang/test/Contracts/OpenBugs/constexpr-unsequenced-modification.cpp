// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-710 in this fork's bug-reports/, llvm#37768; deferred by
// the user 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-748, PR87275).
//
// An unsequenced modification and read of the same scalar is undefined
// ([intro.execution]/10), so f() is not a core constant expression and the
// static_assert must be an error; Clang only warns.  When it is rejected,
// this XFAIL becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-unsequenced-modification.C)

constexpr int f() { int i = 0; i = i++ + i; return i; } // expected-warning {{unsequenced modification and access to 'i'}}
static_assert(f() >= 0); // expected-error {{not an integral constant expression}}
