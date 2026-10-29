// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-json.json -emit-llvm-only -verify %s

// P4405: prove and diagnose selected by a contract configuration, per kind.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-json.C

int f(int x)
  pre(x > 0)                    // expected-note {{the predicate of the contract assertion}}
  post(r: r > 0)                // expected-warning {{contract assertion cannot be proven}}
{
  contract_assert(x != 3);      // expected-warning {{contract assertion cannot be proven}}
  return x;
}

void ae2_calls(int y) {
  f(y); // expected-error {{contract assertion cannot be proven}}
}
