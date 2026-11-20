// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": the overridden function's postconditions
// are proven in each override shadow after all of the overrider's checks,
// with the overridden function's preconditions as premises, also for an
// overrider with no contract assertions of its own.  Unproven, one is
// diagnosed at the overrider's definition, naming the overridden function,
// with a note at the predicate.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-base-post.C

struct B {
  virtual int f(int x) post(r: r > 0);  // expected-note {{the predicate of the contract assertion}}
  virtual int g(int x) pre(x > 0) post(r: r > 0);
  virtual int h(const int x) pre(x > 0) post(r: r >= x);
};

struct D : B {
  int f(int x) override         // expected-error {{contract assertion cannot be proven for a call through 'B::f'}} expected-remark {{override shadow: not proven}}
  { return x; }
  int g(int x) override         // expected-remark {{override shadow: proven, conditional}}
  { return x; }
  int h(int x) override         // expected-remark {{override shadow: proven, conditional}}
  { return x > 5 ? x : x + 1; }
};

// Proven soundly: nothing is given, nothing is relied on.
struct E : B {
  int f(int x) override         // expected-remark {{override shadow: proven, sound}}
  { return x > 0 ? x : 1; }
};

// An overrider with a precondition and no postcondition: both obligations
// in one override shadow.
struct F : B {
  int g(int x) override         // expected-remark 2 {{override shadow: proven, conditional}}
    pre(x >= 1)
  { return x; }
};

void use() { D d; E e; F f; }
