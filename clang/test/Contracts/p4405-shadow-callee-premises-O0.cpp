// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=observe %libcxx_flags -O0 -S -emit-llvm -o %t.on.ll -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 -fcontracts-prove-optimize=0 -fcontract-evaluation-semantic=observe -fno-contracts-prove-callee-premises %libcxx_flags -O0 -S -emit-llvm -o %t.off.ll -Xclang -verify %s
// RUN: diff %t.on.ll %t.off.ll
// RUN: FileCheck %s --input-file=%t.on.ll
// REQUIRES: contracts-libcxx

// D4405 (-fcontracts-prove-callee-premises): at proof level 0 there is no proof
// shadow, so no premise copy, and the option changes nothing: the same
// diagnostics (folding alone proves neither) and the same code.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-callee-premises-O0.C

#include <contracts>
using namespace std::contracts::labels;

int idx(int x) post(r : r == 0) { return x; }
void h1(int x) {
  int i = idx(x);
  contract_assert<diagnose>(i == 0); // expected-warning {{cannot be proven}}
}

inline int idx2(int x) {
  contract_assert(x == 0);
  return x;
}
void h3(int x) {
  int i = idx2(x);
  contract_assert<diagnose>(i == 0); // expected-warning {{cannot be proven}}
}

// CHECK-NOT: contract.premise
