// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-configuration-file=%S/p4405-fold-caller-side-run.json %libcxx_flags -o %t -Xclang -verify
// RUN: %t

// P4405 stage 2: substituting the arguments of a call into a caller-side
// precondition is only ever used for folding.  An argument is evaluated
// exactly once, by the call itself, whether or not the precondition was
// proven, always false or unproven, and the predicate never runs.  diagnose
// is used so that the program links and runs.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-fold-caller-side-run.C

#include <contracts>

static int args, checks, violations;
int arg(int v) { ++args; return v; }
bool chk(bool b) { ++checks; return b; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int divide(int n, int d) pre(d != 0) { return n / (d ? d : 1); } // expected-note 3 {{the predicate of the contract assertion}}
int checked(int d) pre(chk(d != 0)) { return d; } // expected-note {{the predicate of the contract assertion}}

int main() {
  int n = 6;
  divide(n, arg(2));            // expected-warning {{contract assertion cannot be proven}}
  divide(n, (arg(0), 2));       // expected-warning {{contract assertion cannot be proven}}
  divide(n, 0);                 // expected-warning {{contract predicate is always false}}
  divide(n, 3);                 // proven
  checked(arg(0));              // expected-warning {{contract assertion cannot be proven}}
  if (args != 3 || checks != 0 || violations != 0)
    __builtin_abort();
}
