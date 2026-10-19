// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-713 in this fork's bug-reports/, llvm#211286; deferred by
// the user 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-784, PR126357).
//
// Calling a member function of the class under construction in a
// mem-initializer for a base class is undefined ([class.base.init]/16), so
// the static_assert must be an error; Clang accepts it.  A virtual call there
// is rejected (../constexpr-virtual-call-base-mem-init.cpp); the
// dynamic_cast half of the row is constexpr-dynamic-cast-in-base-mem-init.cpp.
// When it is rejected, this XFAIL becomes an XPASS, the signal to close the
// bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-call-in-base-mem-init.C)

struct A { int a; constexpr A(int x) : a(x) {} };
struct B : A {
  constexpr int g() const { return 1; }
  constexpr B() : A(g()) {}
};
static_assert((B{}, true)); // expected-error {{not an integral constant expression}}
