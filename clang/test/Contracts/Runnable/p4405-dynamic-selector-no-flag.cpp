// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p4405-dynamic-selector.json %libcxx_flags -o %t -Xclang -verify
// RUN: %t
// expected-no-diagnostics

// P4405: without -fcontracts-p4405, a P3595 dynamic selector returning
// prove or diagnose has it adjusted to the allowed set, ignore, so it also
// behaves as ignore: the predicate is not evaluated, nothing is reported,
// and nothing is diagnosed at compile time.  A checking value returned
// afterwards still checks.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-dynamic-selector-no-flag.C

#include <contracts>

using std::contracts::evaluation_semantic;

static evaluation_semantic chosen;

extern "C" evaluation_semantic p4405_dynamic_sel() { return chosen; }

static int evaluated, violations;
bool chk(int x) { ++evaluated; return x > 0; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x) pre(chk(x)) post(r: chk(r)) {
  contract_assert(chk(x));
  return x;
}

int main() {
  chosen = evaluation_semantic::prove;
  f(-1);
  chosen = evaluation_semantic::diagnose;
  f(-1);
  if (evaluated != 0 || violations != 0)
    __builtin_abort();
  chosen = evaluation_semantic::observe;
  f(-1);
  if (evaluated != 3 || violations != 3)
    __builtin_abort();
}
