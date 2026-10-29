// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -flto=full -emit-llvm-bc -o %t.full.bc -verify %s
// RUN: llvm-dis %t.full.bc -o - | FileCheck %s --implicit-check-not=contract.proof --implicit-check-not=__clang_contract --implicit-check-not=clang.contract
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -flto=thin -emit-llvm-bc -o %t.thin.bc -verify %s
// RUN: llvm-dis %t.thin.bc -o - | FileCheck %s --implicit-check-not=contract.proof --implicit-check-not=__clang_contract --implicit-check-not=clang.contract
// REQUIRES: x86-registered-target

// P4405 stage 3: under -flto the proof shadow is decided at the compile
// step, before the pre-link pipeline (with the full pipeline of the -O
// level, so it proves what it proves without -flto), and nothing of it
// reaches the bitcode.  A dominating condition is proven there; an
// unguarded evaluation is diagnosed (diagnose, so that the bitcode is
// written).  (The link step needs an LTO-capable
// linker, which these tests do not assume; GCC's p4405-shadow-lto-run.C
// has no mirror.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-lto.C

// CHECK: define {{.*}} @_Z1fii(

int f(int i, int n) {
  if (i < n)
    contract_assert(i < n);
  contract_assert(i < n);       // expected-warning {{contract assertion cannot be proven}}
  return i;
}
