// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fno-contracts-prove-assume -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -O2 -emit-llvm -o %t.diagnose.ll %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fcontract-analysis-semantic=ignore -fno-contracts-prove-assume -triple x86_64-unknown-linux-gnu -fcxx-exceptions -fexceptions -O2 -emit-llvm -o %t.ignore.ll %s
// RUN: diff %t.diagnose.ll %t.ignore.ll
// RUN: FileCheck %s --implicit-check-not=contract.override --implicit-check-not=__clang_contract_unproven --implicit-check-not=__clang_contract_proven --implicit-check-not=clang.contract. < %t.diagnose.ll
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions": nothing of the override shadows -- the
// obligations at entry and at each return, the snapshots of `this', of the
// parameters and of a covariant result -- reaches the real functions,
// which keep their own checks: F::f's precondition and F::own's
// postcondition.  The output is the one every analysis evaluation ignore
// gives.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-realcode.C
// (gnu_gcc scans its optimized dump for the regions; here the IR is
// compared)

struct R { R(); R(const R &); ~R(); int v; };
int sink(int);

struct X { int pad; virtual void x(); };

struct B {
  virtual int f(int x) pre(x > 0) post(r: r > 0);
  virtual int g(int x) noexcept pre(x > 0) post(r: r > 0);
  virtual int h(int x) pre(x > 0) post(r: r > 0);
  virtual R make(int x) pre(x > 0) post(r: r.v > 0);
  virtual B *self() post(r: r == this);
  virtual int p(const int x) post(r: r == x);
  virtual int own(const int x) pre(x > 0) post(r: r > 0);
};

struct F : X, B {
  virtual void key();
  int f(int x) override pre(x >= 1) { return sink(x); }
  int g(int x) noexcept override { return sink(x) + x; }
  int h(int x) override { if (x > 100) throw 1; return sink(x); }
  R make(int x) override { R r; r.v = sink(x); return r; }
  F *self() override { sink(0); return this; }
  int p(int x) override { int r = x; x = sink(x); return r + x; }
  int own(const int x) override post(r: r >= x) { return x + sink(x); }
};

// The key function emits the vtable, and with it the overriders.
void F::key() {}

// CHECK-LABEL: define {{.*}} @_ZN1F1fEi(
// CHECK: __cxa_contract_violation
// CHECK-LABEL: define {{.*}} @_ZN1F3ownEi(
// CHECK: __cxa_contract_violation
