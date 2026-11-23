// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -emit-llvm -o - -verify %s | FileCheck %s

// P4405: the contracts of a virtual function (P3097) under diagnose (as
// under prove).  A postcondition is diagnosed in the function's own body.
// A precondition is analyzed at each call (D4405) -- a virtual call too,
// for the function it names -- and not in the body.  A virtual call builds
// no contract wrapper for them -- they have no run-time effect -- so they
// are diagnosed once.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-virtual.C (with prove;
// GCC builds the wrapper, which evaluates the postcondition again at run
// time; B::h is defined out of line here so that its body is emitted)

struct B {
  virtual int h(int x)
    pre(x > 0)    // expected-note {{the predicate of the contract assertion}}
    post(r: r > 1); // expected-warning {{contract assertion cannot be proven}}
};

int B::h(int x) { return x; }

int call(B &b, int y) {
  int r = b.h(1);
  return r + b.h(y); // expected-warning {{contract assertion cannot be proven}}
}

// CHECK: define{{.*}} @_Z4callR1Bi(
// CHECK-NOT: __contract_wrapper
