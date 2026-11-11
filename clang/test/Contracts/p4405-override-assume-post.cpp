// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p3400 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose %libcxx_flags -O2 -S -emit-llvm -o - -Rcontract-proof -Xclang -verify %s | FileCheck %s
// REQUIRES: contracts-libcxx

// D4405R0, "Virtual Functions": no override proof of a postcondition is
// ever an assumption in the real code (-fcontracts-prove-assume, implied
// by -fcontracts-p4405).  D::f's postcondition is proven in its override
// shadow, from B::f's precondition, but not in D::f's proof shadow, so
// the real D::f keeps its check; B::f's postcondition, proven there too,
// has no check in D::f to drop.  An ordinary sound proof next to override
// shadows is still assumed: D::g's postcondition.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-assume-post.C

#include <contracts>

struct B {
  virtual int f(int x)
    pre<std::contracts::labels::always_enforce>(x > 0)
    post(r: r > 0);
  virtual int g(int x) pre<std::contracts::labels::always_enforce>(x > 0);
};

struct D : B {
  virtual void key();
  // The override shadow relies on D::f's proof shadow's obligation, which
  // is unproven: conditional.
  int f(const int x) override   // expected-remark 2 {{override shadow: proven, conditional}}
    post(r: r > 0)              // expected-warning {{contract assertion cannot be proven}} expected-remark {{proof shadow: not proven}}
  { return x; }
  int g(const int x) override   // expected-remark {{override shadow: proven, sound}}
    post(r: r >= 0)             // expected-remark {{proof shadow: proven, sound; assumed}}
  { return x > 0 ? x : 0; }
};

// The key function emits the vtable, and with it D::f and D::g.
void D::key() {}

// CHECK-LABEL: define linkonce_odr {{.*}}i32 @_ZN1D1fEi(
// CHECK: __cxa_contract_violation
// CHECK: ret i32
// CHECK-NOT: contract.override
