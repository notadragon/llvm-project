// RUN: %clang_cc1 -std=c++26 -fcontracts-p3850 -fcontract-evaluation-semantic=prove -emit-llvm-only -verify %s
// RUN: %clang -### -std=c++26 -fcontracts-p4405 -c %s 2>&1 | FileCheck %s --check-prefix=DRIVER

// P4405: the umbrella -fcontracts-p3850 implies -fcontracts-p4405, which
// implies -fcontracts; the driver passes -fcontracts-p4405 on, with
// -fcontracts.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-umbrella.C (the
// driver line is Clang's own)

#ifndef __cpp_contracts_prove
#error "__cpp_contracts_prove not defined with -fcontracts-p3850"
#endif
#if __cpp_contracts_prove <= 0
#error "__cpp_contracts_prove not positive"
#endif

int f(int x) pre(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  f(y); // expected-error {{contract assertion cannot be proven}}
}

// DRIVER: "-cc1"
// DRIVER-SAME: "-fcontracts"
// DRIVER-SAME: "-fcontracts-p4405"
