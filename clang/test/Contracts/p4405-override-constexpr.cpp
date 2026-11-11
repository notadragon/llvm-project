// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": a constexpr virtual function gets its
// override shadows like any other, and constant evaluation of it is
// unaffected: nothing of an override shadow is in the function's code,
// neither the obligations nor the copy of D::g's `x' that B::g's
// postcondition reads (DECISIONS.md E153), proven.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-constexpr.C

struct B {
  constexpr virtual int f(int x) const pre(x > 0) { return x; }
  constexpr virtual int g(const int x) const post(r: r == x) = 0;
};

struct D : B {
  constexpr int f(int x) const override  // expected-error {{cannot be proven for a call through 'B::f'}}
    pre(x > 10)                          // expected-note {{the predicate of the contract assertion}}
  { return x; }
  constexpr int g(int x) const override
  { int r = x; x = 0; return r; }
};

static_assert(D{}.f(20) == 20);
static_assert(D{}.g(5) == 5);

void use() { D d; }
