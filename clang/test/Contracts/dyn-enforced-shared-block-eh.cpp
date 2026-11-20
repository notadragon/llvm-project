// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/dyn-enforced-shared-block-eh.json %libcxx_flags -o %t && %t

// Under P3595 dynamic selection, a selector value with no arm is an enforced
// violation, and a throwing handler's exception unwinds from the contract that
// reported it: each contract emits its own enforced call rather than sharing
// one built in the first contract's EH scope.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/dyn-enforced-shared-block-eh.C

#include <contracts>
#include <cstdlib>

extern "C" std::contracts::evaluation_semantic my_sel() {
  return static_cast<std::contracts::evaluation_semantic>(1000); // no arm
}

static int dtor = 0, inner = 0, calls = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  if (++calls > 3)   // the misrouted throw loops back here
    std::exit(2);
  throw 1;
}

struct D { ~D() { ++dtor; } };

void g(bool first) {
  if (first) {
    try {
      D d;
      contract_assert(false);
    } catch (int) {
      ++inner;
    }
  }
  contract_assert(false);   // must unwind to main, not to the catch above
}

int main() {
  int outer = 0;
  try { g(true); } catch (int) { outer = 1; }
  if (inner != 1 || dtor != 1 || outer != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
