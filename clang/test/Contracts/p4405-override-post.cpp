// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": an overrider's postconditions are proven
// in each override shadow at each return, ahead of their own checks, with
// the overridden function's preconditions as premises, and again in the
// overrider's proof shadow, without them -- distinct analysis evaluations,
// each diagnosed on its own.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-post.C

// Provable for a call through B, from B::f's precondition, but not for a
// direct call.
namespace through {
  struct B { virtual int f(int x) pre(x > 0); };
  struct D : B {
    int f(const int x) override  // expected-remark {{override shadow: proven, conditional}}
      post(r: r > 0)             // expected-warning {{contract assertion cannot be proven}} expected-remark {{proof shadow: not proven}}
    { return x; }
  };
  void use() { D d; }
}

// An obligation is never proven by its own check: D::f's enforced
// postcondition follows its analysis evaluations, so neither the override
// shadow (B::f has no contract assertions, so it gives no premise:
// DECISIONS.md E156) nor the proof shadow proves it.
namespace own {
  struct B { virtual int f(int x); };
  struct D : B {
    int f(const int x) override  // expected-warning {{contract assertion cannot be proven for a call through 'own::B::f'}} expected-remark {{override shadow: not proven}}
      post(r: r > 0)             // expected-warning {{contract assertion cannot be proven}} expected-remark {{proof shadow: not proven}} expected-note {{the predicate of the contract assertion}}
    { return x; }
  };
  void use() { D d; }
}

// B::f's postcondition is proven after D::f's enforced check of the same
// predicate, which comes first -- conditionally, relying on D::f's
// obligations, unproven; D::f's postcondition itself is not proven.
namespace order {
  struct B { virtual int f(int x) post(r: r > 0); };
  struct D : B {
    int f(const int x) override  // expected-warning {{contract assertion cannot be proven for a call through 'order::B::f'}} expected-remark {{override shadow: not proven}} expected-remark {{override shadow: proven, conditional}}
      post(r: r > 0)             // expected-warning {{contract assertion cannot be proven}} expected-remark {{proof shadow: not proven}} expected-note {{the predicate of the contract assertion}}
    { return x; }
  };
  void use() { D d; }
}
