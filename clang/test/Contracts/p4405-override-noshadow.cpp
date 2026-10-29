// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fno-contracts-prove-shadow -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", with -fno-contracts-prove-shadow alone:
// the override shadows are a proving strategy of their own
// (-fcontracts-prove-override, DECISIONS.md E157), so D::f's precondition
// is still proven from B::f's in its override shadow, while an ordinary
// evaluation, D::f's contract_assert, is settled by folding alone.  E::f's
// precondition folds, so it needs no override shadow: one remark.  Without
// either strategy, see p4405-override-nooverride-noshadow.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-noshadow.C

struct B { virtual int f(int x) pre(x > 0); };

struct D : B {
  int f(int x) override         // expected-remark {{override shadow: proven, conditional}}
    pre(x >= 0)
  {
    contract_assert(x >= 0);    // expected-error {{contract assertion cannot be proven}}
    return x;
  }
};

struct E : B {
  int f(int x) override pre(true) { return x; }
};

void use() { D d; E e; }
