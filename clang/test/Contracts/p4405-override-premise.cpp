// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=prove %libcxx_flags -O2 -S -emit-llvm -o /dev/null -Rcontract-proof -Xclang -verify %s
// REQUIRES: contracts-libcxx

// D4405R0, "Virtual Functions", DECISIONS.md E151: a proof in an override
// shadow relies on the overridden function's preconditions, which every
// caller evaluates -- in any translation unit, with that unit's semantic
// -- so they are guaranteed only when their labels admit nothing but
// terminating semantics and assume.  D::g's precondition is then proven
// soundly, D::f's only conditionally; neither is diagnosed.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-premise.C

#include <contracts>

struct B {
  virtual int f(int x) pre(x > 0);
  virtual int g(int x) pre<std::contracts::labels::always_enforce>(x > 0);
};

struct D : B {
  int f(int x) override pre(x >= 1) { return x; }  // expected-remark {{override shadow: proven, conditional}}
  int g(int x) override pre(x >= 1) { return x; }  // expected-remark {{override shadow: proven, sound}}
};

void use() { D d; }
