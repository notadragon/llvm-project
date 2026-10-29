// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-caller-side.json -triple x86_64-unknown-linux-gnu -emit-llvm-only -verify %s

// P4405: a caller-side evaluation with prove or diagnose is diagnosed at the
// call, once per call, with a note at the predicate; the callee-side ones
// here are ignore, so the definitions are not diagnosed.  A call in an
// unevaluated operand evaluates nothing and is not diagnosed.  Caller-side
// checking is otherwise not implemented in Clang, but these two need no
// check, so the configuration draws no warning that it is ignored.
// Constructor and member function calls are diagnosed the same way, an
// inheriting constructor's call too.  No
// argument is a constant, so folding with the arguments substituted
// (p4405-fold-caller-side.cpp) proves none of them.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-caller-side.C

int f(int x)
  pre(x > 0)            // expected-note 2 {{the predicate of the contract assertion}}
  post(r: r > 0);       // expected-note 2 {{the predicate of the contract assertion}}

int g(int x) {
  return f(x);          // expected-error {{contract assertion cannot be proven}} \
                        // expected-warning {{contract assertion cannot be proven}}
}

decltype(f(1)) h();

int k(int x) {
  return f(x + 1);      // expected-error {{contract assertion cannot be proven}} \
                        // expected-warning {{contract assertion cannot be proven}}
}

struct S {
  S(int x) pre(x > 0);  // expected-note 2 {{the predicate of the contract assertion}}
  void m(int x) pre(x > 0); // expected-note {{the predicate of the contract assertion}}
};

// A call of an inheriting constructor evaluates the inherited constructor's
// precondition, diagnosed at that call (p4405-caller-side-inheriting.cpp
// has more, mirrored in gnu_gcc since GCC-888's fix).
struct T : S {
  using S::S;
};

void use(int x) {
  S s(x);               // expected-error {{contract assertion cannot be proven}}
  s.m(x);               // expected-error {{contract assertion cannot be proven}}
  T t(x);               // expected-error {{contract assertion cannot be proven}}
}
