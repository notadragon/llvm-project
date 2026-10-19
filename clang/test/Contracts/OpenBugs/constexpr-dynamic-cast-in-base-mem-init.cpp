// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-713 in this fork's bug-reports/, llvm#211286, which covers
// this form only partly; deferred by the user 2026-10-12, DECISIONS.md E73).
// Stock Clang and GCC alike (gnu_gcc GCC-784, PR126357).
//
// Applying dynamic_cast to `this` in a mem-initializer for a base class is
// undefined ([class.base.init]/16), so the static_assert must be an error;
// Clang accepts it.  The member call half of the row is
// constexpr-call-in-base-mem-init.cpp.  When it is rejected, this XFAIL
// becomes an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-call-in-base-mem-init.C)

struct P { bool p; constexpr P(bool x) : p(x) {} virtual void f() {} };
struct Q : P {
  constexpr Q() : P(dynamic_cast<Q *>(this) != nullptr) {}
};
static_assert((Q{}, true)); // expected-error {{not an integral constant expression}}
