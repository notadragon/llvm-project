// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -triple x86_64-unknown-linux-gnu -O2 -mconstructor-aliases -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E152 and E157: without
// override shadows (-fno-contracts-prove-override), a virtual destructor --
// an implicitly defined one too -- must establish the overridden
// destructors' postconditions as obligations of its own, proven in its
// proof shadow.  In a class with virtual bases the complete-object
// destructor holds them.  The diagnostics still name the overridden
// destructor.  (The runtime evaluations are observe, as in
// p4405-override-dtor.cpp, which makes the proofs relying on B::~B's code
// conditional.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-dtor-nooverride.C

int g;

struct B {
  virtual ~B()
    post(g == 1)                // expected-remark {{proof shadow: proven, conditional}}
  { g = 1; }
};

struct D : B {                  // expected-remark {{proof shadow: proven, conditional}}
};

int g2;

struct B2 {
  virtual ~B2() post(g2 == 1);  // expected-note 2 {{the predicate of the contract assertion}}
};

struct E : B2 {
  ~E() override {}              // expected-error {{cannot be proven for a call through 'B2::~B2'}} expected-remark {{proof shadow: not proven}}
};

struct F : B2 {                 // expected-error {{cannot be proven for a call through 'B2::~B2'}} expected-remark {{proof shadow: not proven}}
};

struct V : virtual B {          // expected-remark {{proof shadow: proven, conditional}}
};

void use() { D d; E e; F f; V v; }
