// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s
// expected-no-diagnostics

// A pointer to a member of C converted to a pointer to member of its base Bb
// ([expr.static.cast]/12) may be applied to an object that contains the
// member, here a C, so this is a constant expression.  Clang accepts it; GCC
// rejects it (stock, gnu_gcc GCC-715, rejects-valid, deferred), and its
// watch test is the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-ptrmem-cast-to-base-rejects-valid.C

struct Bb { int i; };
struct C : Bb { int m; };
constexpr int f() {
  int Bb::*p = static_cast<int Bb::*>(&C::m);
  C c{};
  c.m = 5;
  return c.*p;
}
static_assert(f() == 5);
