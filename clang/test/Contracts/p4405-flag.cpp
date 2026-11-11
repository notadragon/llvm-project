// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -emit-llvm-only -verify %s

// P4405: -fcontracts-p4405 defines __cpp_contracts_prove (checked defined
// and positive only) and accepts the prove and diagnose semantics.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-flag.C

#ifndef __cpp_contracts_prove
#error "__cpp_contracts_prove not defined with -fcontracts-p4405"
#endif
#if __cpp_contracts_prove <= 0
#error "__cpp_contracts_prove not positive"
#endif

int f(int x) pre(x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}

void ae2_calls(int y) {
  f(y); // expected-warning {{contract assertion cannot be proven}}
}
