// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": the paper's example.  For each function
// B::f that D::f overrides, an override shadow of D::f takes B::f's
// preconditions as given and proves, in order, D::f's preconditions at
// entry, D::f's postconditions and B::f's postconditions at each return.
// D::f's `x >= 0` follows from B::f's `x > 0`, and both postconditions
// from the body -- conditionally, since B::f's precondition is evaluated
// by the callers, which may ignore or observe it (DECISIONS.md E151) --
// while E::f's `x > 10` does not follow, so the program is ill-formed,
// diagnosed at E::f's definition naming the overridden function; B::f's
// `r > 0` is proven for E::f too.  D::f's own postcondition is proven in
// its proof shadow as well.  Nothing calls any of them, so no precondition
// has an analysis evaluation at a call; make() emits the vtables, and with
// them the overriders.  The paper's D::f declares `x` without const, which
// a postcondition naming it requires.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-paper.C

struct B {
  virtual int f(int x)
    pre(x > 0)
    post(r: r > 0);
};

struct D : B {
  int f(const int x) override  // expected-remark 3 {{override shadow: proven, conditional}}
    pre(x >= 0)
    post(r: r > x)             // expected-remark {{proof shadow: proven, sound}}
  {
    return x + 1;
  }
};

struct E : B {
  int f(int x) override        // expected-error {{contract assertion cannot be proven for a call through 'B::f'}} expected-remark {{override shadow: not proven}} expected-remark {{override shadow: proven, conditional}}
    pre(x > 10)                // expected-note {{the predicate of the contract assertion}}
  {
    return x;
  }
};

B *make(bool d) { return d ? static_cast<B *>(new D) : new E; }
