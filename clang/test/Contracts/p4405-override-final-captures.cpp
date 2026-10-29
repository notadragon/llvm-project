// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": `final' changes nothing about an
// overrider's own override shadows (it only stops later overriders), and a
// postcondition with captures (P3098), of the overridden function or of the
// overrider, is decided at once for every override pair, as its own
// analysis evaluation is: its captures are initialized only for a checked
// evaluation, so only folding can settle it.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-final-captures.C

int cap(int);

struct B {
  virtual int f(int x) post [c = cap(x)] (r: r >= c);  // expected-note {{the predicate of the contract assertion}}
  virtual int g(int x) pre(x > 0);
  virtual int h(int x) pre(x > 0) post [c = 1] (r: r >= c);  // expected-note {{the predicate of the contract assertion}}
};

struct D final : B {
  int f(int x) override         // expected-error {{cannot be proven for a call through 'B::f'}}
  { return cap(x); }
  int g(int x) override final   // expected-remark {{override shadow: proven, conditional}}
    pre(x > -1)
  { return x; }
};

struct E : B {
  // Both postconditions have captures: E::h's own analysis evaluation, and
  // E::h's and B::h's obligations for the pair, are each decided at once,
  // unproven, although B::h's precondition would give `r >= 1'.
  int h(int x) override         // expected-error 2 {{cannot be proven for a call through 'B::h'}}
    post [d = x] (r: r == d)    // expected-error {{contract assertion cannot be proven}} expected-note {{the predicate of the contract assertion}}
  { return x; }
};

void use() { D d; E e; }
