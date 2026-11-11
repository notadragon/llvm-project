// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fno-contracts-prove-shadow -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", with neither override shadows nor proof
// shadows (-fno-contracts-prove-override -fno-contracts-prove-shadow,
// DECISIONS.md E157): every override obligation is decided at once, so
// only folding settles one, whether the overrider's precondition or the
// overridden function's postcondition.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-nooverride-noshadow.C

struct B {
  virtual int f(int x) pre(x > 0) post(r: r > 0);  // expected-note {{the predicate of the contract assertion}}
  virtual int g(int x) post(r: true);
};

struct D : B {
  int f(int x) override         // expected-warning 2 {{cannot be proven for a call through 'B::f'}}
    pre(x > 0)                  // expected-note {{the predicate of the contract assertion}}
  { return 1; }
  int g(int x) override { return x; }
};

void use() { D d; }
