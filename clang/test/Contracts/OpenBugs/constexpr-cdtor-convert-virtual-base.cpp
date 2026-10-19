// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify -verify-ignore-unexpected=note %s
// XFAIL: *

// OPEN BUG (CLANG-714 in this fork's bug-reports/; deferred by the user
// 2026-10-12, DECISIONS.md E73).  Stock Clang and GCC alike (gnu_gcc
// GCC-759).
//
// Converting `this` to a virtual base A of DBase before DBase's construction
// has begun is undefined ([class.cdtor]/3), so t() is not a core constant
// expression; Clang accepts it.  The dynamic_cast form of the row is
// constexpr-cdtor-dynamic-cast.cpp.  When it is rejected, this XFAIL becomes
// an XPASS, the signal to close the bug.
// (GCC mirror: g++.dg/contracts/cpp26/open-bug-constexpr-cdtor-convert-typeid-virtual.C)

struct A { int tag = 1; };
struct CBase : virtual A {};
struct DBase : virtual A { A *p; constexpr DBase(A *a) : p(a) {} };
struct E : CBase, DBase {
  constexpr E() : DBase(this) {} // DBase not yet constructed
};
constexpr int t() { E e; return e.p == static_cast<A *>(&e); }
static_assert(t()); // expected-error {{not an integral constant expression}}
