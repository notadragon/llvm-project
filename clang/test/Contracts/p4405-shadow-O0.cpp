// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3: at -O0 there is no proof shadow, so folding is final: the
// evaluations a shadow would prove at -O2 are diagnosed, each once, and
// one that folds is still proven.  A function's own optnone attribute
// decides for it (p4405-shadow-optnone.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-O0.C

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);     // expected-error {{contract assertion cannot be proven}}
  contract_assert(2 > 1);
  int c = n < 0 ? 0 : 1;
  contract_assert(c >= 0);      // expected-error {{contract assertion cannot be proven}}
  return c;
}
