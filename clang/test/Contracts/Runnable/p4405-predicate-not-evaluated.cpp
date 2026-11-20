// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %libcxx_flags -o %t
// RUN: %t

// P4405: prove and diagnose have no run-time effect: the predicate of a
// precondition, a postcondition or an assertion is never evaluated and no
// violation is reported.  diagnose is used, silenced, so that the program
// links and runs.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-predicate-not-evaluated.C

#include <contracts>

static int evaluated, violations;
bool chk(int x) { ++evaluated; return x > 0; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x) pre(chk(x)) post(r: chk(r)) {
  contract_assert(chk(x));
  return x;
}

struct S {
  int v;
  S(int x) pre(chk(x)) : v(x) {}
  ~S() { contract_assert(chk(v)); }
};

int main() {
  f(-1);
  { S s(-1); }
  if (evaluated != 0 || violations != 0)
    __builtin_abort();
}
