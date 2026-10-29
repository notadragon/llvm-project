// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A static_cast of a pointer to member to a base class that does not contain
// the member is undefined ([expr.static.cast]/12), so it is not a core
// constant expression.  Clang rejects it; GCC accepts it (stock, gnu_gcc
// GCC-742, PR109243, deferred), and its watch test is the program below.
// Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-ptrmem-cast-unrelated-base.C

struct A { int a; };
struct B { int b; };
struct D : A, B {};
constexpr bool f() {
  int D::*pd = &A::a;
  int B::*pb = static_cast<int B::*>(pd); // expected-note {{subexpression not valid in a constant expression}}
  return pb != nullptr;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
