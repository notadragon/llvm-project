// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// For a member of a class template that is virtual at its definition,
// "contracts on virtual functions require '-fcontracts-p3097'" is issued once,
// for the pattern, not again for the instantiation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/virtual-flag-error-twice.C

struct B { virtual void f(int) = 0; };
template <class T> struct S : B {
  T m = 0;
  void f(int x) override pre(x > m) {} // expected-error {{contracts on virtual functions require '-fcontracts-p3097'}}
};
void use() {
  S<int> s;
  s.f(1);
}
