// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": an override obligation analyzed with
// diagnose is a -Wcontract-unproven warning, at the overrider's definition
// and naming the overridden function; "always false" when the overrider's
// precondition folds to false.  (An override shadow relies on the
// overridden function's preconditions, which a caller may not establish,
// so its own answer is never "always false" unless their labels settle
// them.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-diagnose.C

struct B { virtual int f(int x) pre(x > 0); };

struct D : B {
  int f(int x) override         // expected-warning {{contract assertion cannot be proven for a call through 'B::f'}}
    pre(x > 10)                 // expected-note {{the predicate of the contract assertion}}
  { return x; }
};

struct E : B {
  int f(int x) override         // expected-warning {{contract predicate is always false for a call through 'B::f'}}
    pre(false)                  // expected-note {{the predicate of the contract assertion}}
  { return x; }
};

struct F : B {
  int f(int x) override pre(x > -5) { return x; }
};

void use() { D d; E e; F f; }
