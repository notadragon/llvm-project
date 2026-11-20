// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// P4405 stage 3, -fcontracts-prove-shadow: as p4405-shadow-prove.cpp, with
// diagnose: an evaluation the proof shadow proves is not diagnosed, one it
// does not prove is a -Wcontract-unproven warning.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-diagnose.C

int f(int x)
  pre(x != 0)                   // expected-note {{the predicate of the contract assertion}}
{
  if (x > 0)
    contract_assert(x != 0);
  contract_assert(x > 0);       // expected-warning {{contract assertion cannot be proven}}
  return x;
}

void ae2_calls1(int y) { f(y); } // expected-warning {{contract assertion cannot be proven}}
