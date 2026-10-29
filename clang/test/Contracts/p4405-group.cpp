// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-group-evaluation-semantic=g:prove,h:diagnose -fcontract-evaluation-semantic=ignore -emit-llvm-only -verify %s

// P4405: prove and diagnose selected by -fcontract-group-evaluation-semantic=
// for the groups of a contract assertion; a contract in neither group keeps
// the command-line semantic.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-group.C (which names
// the groups with assertion-control labels; [[clang::contract_group]] here)

int f(int x) pre [[clang::contract_group("g")]] (x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}
int g(int x) pre [[clang::contract_group("h")]] (x > 0) { return x; } // expected-note {{the predicate of the contract assertion}}
int h(int x) pre(x > 0) { return x; }

void ae2_calls(int y) {
  f(y); // expected-error {{contract assertion cannot be proven}}
  g(y); // expected-warning {{contract assertion cannot be proven}}
  h(y);
}
