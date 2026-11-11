// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-configuration-file=%S/p3098-dynamic-capture.json %libcxx_flags -o %t && %t

// A postcondition with captures whose semantic a P3595 dynamic selector
// chooses at run time chooses it once, where the captures are initialized.
// Under ignore the evaluation has no effect (P3098 [basic.contract.eval]),
// so the capture initializer does not run; under observe it runs, the
// postcondition is checked with that same semantic, and the selector is not
// called again; an invalid selection is an enforced violation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-dynamic-capture.C

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int inits, dtors, calls, observed;
struct G {
  G() { ++inits; }
  G(const G &) { ++inits; }
  ~G() { ++dtors; }
};

static evaluation_semantic chosen = evaluation_semantic::ignore;
evaluation_semantic sel() { ++calls; return chosen; }

void handle_contract_violation(const std::contracts::contract_violation &v) {
  if (v.semantic() == evaluation_semantic::observe)
    ++observed;
  else if (v.semantic() == evaluation_semantic::enforce && calls == 3)
    std::_Exit(0); // the invalid selection, as expected
  else
    __builtin_abort();
}

int f(int i) post [g = G()] (false) { return i; }

static void expect(int i, int d, int c, int o) {
  if (inits != i || dtors != d || calls != c || observed != o)
    __builtin_abort();
}

int main() {
  f(1);
  expect(0, 0, 1, 0); // ignore: no effect

  chosen = evaluation_semantic::observe;
  f(1);
  expect(1, 1, 2, 1); // observe: one selection, one report

  chosen = static_cast<evaluation_semantic>(42);
  f(1); // invalid: enforced violation, exits
  __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
