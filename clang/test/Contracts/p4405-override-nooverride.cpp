// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -Rcontract-proof -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fcontracts-p4405 -fno-contracts-prove-override -fcontract-analysis-semantic=diagnose -Wno-contract-unproven -triple x86_64-unknown-linux-gnu -O2 -emit-llvm -o - %s | FileCheck %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fno-contracts-prove-override -c %s 2>&1 | FileCheck --check-prefix=DRIVER %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -fno-contracts-prove-override -fcontracts-prove-override -c %s 2>&1 | FileCheck --check-prefix=DRIVER-ON %s
// REQUIRES: x86-registered-target

// D4405R0, "Virtual Functions", without override shadows
// (-fno-contracts-prove-override, DECISIONS.md E157): the overrider must
// establish the overridden functions' postconditions as well as its own,
// with no premise from their preconditions -- obligations of its own,
// proven in its proof shadow -- and its preconditions must be tautologies.
// The diagnostics still name the overridden function.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-override-nooverride.C

struct B {
  virtual int f(int x) pre(x > 0) post(r: r > 0);  // expected-note {{the predicate of the contract assertion}}
  virtual int g(int x) pre(x > 0) post(r: r > 0);
  virtual int h(int x) pre(x > 0);
};

struct D : B {
  // Proven with an override shadow, from B::f's precondition; not
  // without one.
  int f(int x) override         // expected-error {{cannot be proven for a call through 'B::f'}} expected-remark {{proof shadow: not proven}}
  { return x; }
  // Proven without B::g's precondition.
  int g(int x) override         // expected-remark {{proof shadow: proven, sound}}
  { return x > 0 ? x : 1; }
  // A tautology, settled by folding.
  int h(int x) override pre(true) { return x; }
};

void use() { D d; }

// The driver passes the flag on, the last one winning.
// DRIVER: "-fno-contracts-prove-override"
// DRIVER-ON: "-fcontracts-prove-override"
// DRIVER-ON-NOT: "-fno-contracts-prove-override"

// No override shadow, and nothing of the proof shadows in the output.
// CHECK-NOT: contract.override
// CHECK-NOT: contract.proof
