// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Werror=contract-unproven -emit-llvm-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only %s 2>&1 | FileCheck %s --check-prefix=GROUP

// P4405: -Werror=contract-unproven makes the diagnose semantic's warning an
// error; the warning names its group.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-werror.C

int f(int x) pre(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  f(y); // expected-error {{contract assertion cannot be proven}}
}

// GROUP: warning: contract assertion cannot be proven [-Wcontract-unproven]
