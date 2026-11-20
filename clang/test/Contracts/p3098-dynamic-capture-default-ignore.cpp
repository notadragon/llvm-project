// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-configuration-file=%S/p3098-dynamic-capture-default-ignore.json %libcxx_flags -o %t && %t

// A dynamically selected postcondition whose static (weak default) semantic
// is ignore still initializes its captures, and is checked, when the selector
// chooses a semantic that evaluates it.

#include <contracts>

using std::contracts::evaluation_semantic;

static int inits, observed;
struct G {
  G() { ++inits; }
  G(const G &) { ++inits; }
};

evaluation_semantic sel() { return evaluation_semantic::observe; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++observed;
}

int f(int i) post [g = G()] (false) { return i; }

int main() {
  f(1);
  if (inits != 1 || observed != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
