// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", without override shadows
// (-fno-contracts-prove-override, DECISIONS.md E157): each override
// obligation is one of the overrider itself, with no premise from the
// overridden function.  D::f's precondition `x >= 0`, proven in its
// override shadow from B::f's `x > 0` (p4405-override-paper.cpp), must now
// be a tautology, and is diagnosed; B::f's postcondition `r > 0` is an
// obligation in D::f's proof shadow, proven there from D::f's own
// precondition, enforced at entry, and in E::f's from E::f's.  D::f's own
// postcondition needs nothing beyond its analysis evaluation.  No override
// shadow is made.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-paper-nooverride.C

struct B {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0);
};

struct D : B {
  int f(const int x) override  // expected-error {{contract assertion cannot be proven for a call through 'B::f'}} expected-remark {{proof shadow: proven, sound}}
    pre(x >= 0)                // expected-note {{the predicate of the contract assertion}}
    post(r: r > x)             // expected-remark {{proof shadow: proven, sound}}
  {
    return x + 1;
  }
};

struct E : B {
  int f(int x) override        // expected-error {{contract assertion cannot be proven for a call through 'B::f'}} expected-remark {{proof shadow: proven, sound}}
    pre(x > 10)                // expected-note {{the predicate of the contract assertion}}
  {
    return x;
  }
};

B *make(bool d) { return d ? static_cast<B *>(new D) : new E; }
