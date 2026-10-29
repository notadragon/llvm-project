// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", the paper's example of overriding the
// functions of two bases, at -O0: no override shadow and no proof shadow,
// so folding is the only strategy and the override obligations are those
// of -fno-contracts-prove-override (DECISIONS.md E157), each decided at
// once.  None of D::f's preconditions and postconditions is a tautology,
// so each of D::f's override obligations is diagnosed for both B1::f and
// B2::f, as are E::f's for B1::f, besides D::f's own postcondition and
// both calls of B1::f in g.  The other errors in the translation unit do
// not hide the override diagnostics.  Clang diagnoses D::f's own
// postcondition at the assertion, with no separate predicate note.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-paper-O0.C

struct B1 {
  virtual int f(int x)
    pre(x >= 0 && x <= 10)      // expected-note 2 {{the predicate of the contract assertion}}
    post(r: r >= 0 && r <= 10); // expected-note 2 {{the predicate of the contract assertion}}
};

struct B2 {
  virtual int f(int x)
    pre(x >= 20 && x <= 30)
    post(r: r >= 20 && r <= 30); // expected-note {{the predicate of the contract assertion}}
};

struct D : B1, B2 {
  int f(const int x) override  // expected-error 2 {{contract assertion cannot be proven for a call through 'B1::f'}} expected-error 2 {{contract assertion cannot be proven for a call through 'B2::f'}}
    pre(x >= 0 && x <= 30)     // expected-note 2 {{the predicate of the contract assertion}}
    post(r: r == x)            // expected-error {{contract assertion cannot be proven}}
  {
    return x;
  }
};

struct E : B1 {
  int f(int x) override        // expected-error 2 {{contract assertion cannot be proven for a call through 'B1::f'}}
    pre(x >= 5 && x <= 10)     // expected-note {{the predicate of the contract assertion}}
  {
    return x;
  }
};

void g(B1 &b, int y) {
  b.f(5);
  b.f(y);                      // expected-error {{contract assertion cannot be proven}}
  if (y >= 0 && y <= 10) {
    b.f(y);                    // expected-error {{contract assertion cannot be proven}}
  }
}

D d;
E e;
