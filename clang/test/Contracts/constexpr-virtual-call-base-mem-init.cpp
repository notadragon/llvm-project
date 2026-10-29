// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// Calling a member function of the class under construction from a
// mem-initializer for a base class is undefined ([class.base.init]/16), so
// it is not a core constant expression.  Clang rejects the virtual call; GCC
// accepts it (stock, gnu_gcc GCC-758, deferred), and its watch test is the
// program below.  The non-virtual call, which Clang accepts too, is CLANG-713
// (OpenBugs/constexpr-call-in-base-mem-init.cpp).  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-virtual-call-base-mem-init.C

struct A { int a; constexpr A(int x) : a(x) {} };
struct B : A {
  constexpr virtual int vf() const { return 1; }
  constexpr B() : A(vf()) {} // expected-note {{subexpression not valid in a constant expression}}
};
constexpr int f() { B b; return b.a; } // expected-note {{in call to 'B()'}}
static_assert(f() == 1); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
