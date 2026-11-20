// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A friend declared in a class is in that class's scope, so an unqualified
// name in its contract predicate finds the class's members
// ([basic.lookup.unqual]), whether the friend is defined in the class or
// outside it, and a class template's friend defined in it.
// (GCC mirror: g++.dg/contracts/cpp26/friend-contract-class-scope.C, GCC-630.)

struct S {
  static constexpr int limit = 10;
  int m(int x) pre(x != limit) { return x; }
  friend int declared(int x) pre(x != limit);
  friend int defined(int x) pre(x != limit) { return x; }
};

int declared(int x) { return x; }

template <class T>
struct U {
  static constexpr T limit = 10;
  friend T tdefined(U, T x) pre(x != limit) { return x; }
};

U<int> u;
int a = declared(1) + tdefined(u, 3);
