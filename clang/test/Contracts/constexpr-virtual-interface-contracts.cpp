// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s

// Under P3097 a virtual call checks the contracts of the statically named
// function (the interface) as well as the final overrider's, in constant
// evaluation as at run time: B::f's violated precondition and B::g's violated
// postcondition are diagnosed though D's overriders have no contracts.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-virtual-interface-contracts.C

struct B {
  constexpr virtual int f(int x) pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
  constexpr virtual int g(int x) post(r: r > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
};
struct D : B {
  constexpr int f(int x) override { return x; }
  constexpr int g(int x) override { return x; }
};
constexpr int t() { D d; B &b = d; return b.f(-1); } // expected-note 0+ {{}}
constexpr int v = t(); // expected-error {{constexpr variable 'v' must be initialized by a constant expression}} expected-note 0+ {{}}
constexpr int u() { D d; B &b = d; return b.g(-1); } // expected-note 0+ {{}}
constexpr int w = u(); // expected-error {{constexpr variable 'w' must be initialized by a constant expression}} expected-note 0+ {{}}
constexpr int ok() { D d; B &b = d; return b.f(1) + b.g(1); }
static_assert(ok() == 2);
