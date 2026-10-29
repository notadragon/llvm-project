// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only \
// RUN:   -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:class.abstract.pure.virtual:enforce
// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// P3100: a constexpr pure virtual function with a definition, called without
// qualification in its class's constructor, is a virtual call to a pure
// virtual function for the object under construction, which [class.abstract]
// makes undefined, and the constant evaluator rejects it -- configured or
// not.  GCC runs the definition instead (stock; gnu_gcc GCC-760, deferred),
// and its mirror pins that.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-pure-virtual-direct-constexpr.C)

struct B {
  int v = 0;
  constexpr B() { v = f(); } // expected-warning {{call to pure virtual member function 'f' has undefined behavior}} \
                             // expected-note {{pure virtual function 'B::f' called}}
  constexpr virtual int f() = 0; // expected-note {{'f' declared here}} \
                                 // expected-note {{declared here}}
  constexpr virtual ~B() {}
};
constexpr int B::f() { return 1; }
struct D : B { // expected-note {{in call to 'B()'}}
  constexpr int f() override { return 2; }
};
constexpr int g() {
  D d; // expected-note {{in call to 'D()'}}
  return d.v * 10 + d.f();
}
static_assert(g() == 12); // expected-error {{static assertion expression is not an integral constant expression}} \
                          // expected-note {{in call to 'g()'}}
