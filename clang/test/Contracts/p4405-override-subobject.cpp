// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=prove -fcontract-evaluation-semantic=observe -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", DECISIONS.md E154: when a base class
// occurs as more than one subobject, an overrider gets one override shadow
// per subobject if the overridden function's contract assertions name
// `this' -- they then read a different object in each -- and one per
// overridden function otherwise.  A virtual base is one subobject however
// often it is reached.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-subobject.C
// (gnu_gcc counts its override shadows in a dump; here each obligation's
// remark)

struct B {
  int m;
  virtual int f() pre(m > 0) post(r: r == m);  // expected-note 3 {{the predicate of the contract assertion}}
  virtual int g(int x) pre(x > 0) post(r: r > 0);
  virtual int h();
};

struct C1 : B {};
struct C2 : B {};

struct D : C1, C2 {
  // Proven for the B in C1, not for the B in C2: two override shadows,
  // one error.
  int f() override              // expected-error {{cannot be proven for a call through 'B::f'}} expected-remark {{override shadow: proven, conditional}} expected-remark {{override shadow: not proven}}
  { return C1::m; }
  // Nothing names `this': one override shadow.
  int g(int x) override { return x; }  // expected-remark {{override shadow: proven, conditional}}
  // B::h has no contract assertions, so nothing names `this': one pair,
  // whose only obligation, D::h's precondition (DECISIONS.md E156), is
  // settled by folding.
  int h() override pre(true) { return 0; }
};

// A virtual base reached twice is one subobject.
struct V1 : virtual B {};
struct V2 : virtual B {};

struct W : V1, V2 {
  int f() override { return m; }  // expected-remark {{override shadow: proven, conditional}}
  int g(int x) override { return x; }  // expected-remark {{override shadow: proven, conditional}}
};

// A base class occurring as a non-virtual and a virtual base: two pairs,
// both unproven -- the non-virtual one's rightly; the virtual one's because
// its two conversions of `this' read the virtual base's offset from
// different vtable pointers, which the optimizers cannot equate.
struct X : C1, V1 {
  int f() override              // expected-error 2 {{cannot be proven for a call through 'B::f'}} expected-remark 2 {{override shadow: not proven}}
  { return V1::m; }
};

void use() { D d; W w; X x; }
