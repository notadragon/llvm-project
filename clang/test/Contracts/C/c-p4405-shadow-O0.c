// RUN: %clang_cc1 -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=prove -triple x86_64-unknown-linux-gnu -O0 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3 in C: at -O0 there is no proof shadow, so folding is
// final.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-shadow-O0.c

int f(int i, int n) {
  if (i < n)
    _ContractAssert(i < n);     // expected-error {{contract assertion cannot be proven}}
  _ContractAssert(2 > 1);
  return i;
}
