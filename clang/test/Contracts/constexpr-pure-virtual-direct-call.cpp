// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A virtual call to a pure virtual function from its class's constructor is
// undefined ([class.abstract]/6) even when the function has a definition, so
// it is not a core constant expression.  Clang rejects it; GCC calls the
// definition, A::f (stock, gnu_gcc GCC-760, deferred), and its watch test is
// the program below.  p3100-pure-virtual-direct-constexpr.cpp pins the same
// with the P3100 check configured.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-pure-virtual-direct-call.C

struct A {
  constexpr virtual int f() const = 0; // expected-note {{'f' declared here}} \
                                       // expected-note {{declared here}}
  int v;
  constexpr A() : v(f()) {} // expected-warning {{call to pure virtual member function 'f' has undefined behavior}} \
                            // expected-note {{pure virtual function 'A::f' called}}
};
constexpr int A::f() const { return 1; }
struct B : A { constexpr int f() const override { return 2; } }; // expected-note {{in call to 'A()'}}
constexpr int g() { B b; return b.v; } // expected-note {{in call to 'B()'}}
static_assert(g() == 1); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'g()'}}
