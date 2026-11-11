// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-selector-throws.json %libcxx_flags -o %t
// RUN: %t
// RUN: %clangxx -std=c++26 -O2 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-selector-throws.json %libcxx_flags -o %t.O2
// RUN: %t.O2

// P3595: the dynamic selector is an ordinary function, so an exception from
// it unwinds like any other: through the handlers and the cleanups of the
// function evaluating the contract (CLANG-835).  Both places a selector is
// called: the dispatch of a contract_assert, and the selection a
// postcondition with captures makes where the captures are initialized.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-selector-throws*.C

#include <contracts>
#include <cstdlib>

using std::contracts::evaluation_semantic;

static int dtors, caught;

struct D {
  ~D() { ++dtors; }
};

evaluation_semantic p3595_throwing_sel() { throw 1; }

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::abort(); // the selector throws before any check
}

// A handler in the same function.
void catches_itself(int x) {
  D d;
  try {
    contract_assert(x > 0);
  } catch (int) {
    ++caught;
  }
}

// No handler: the local's destructor runs on the way out.
void unwinds(int x) {
  D d;
  contract_assert(x > 0);
}

// The selection made where the captures are initialized.
int captures(const int x) post [d = D()] (x > 0) {
  D local;
  return x;
}

int main() {
  catches_itself(1);
  if (caught != 1 || dtors != 1)
    std::abort();
  try {
    unwinds(1);
    std::abort();
  } catch (int) {
  }
  if (dtors != 2)
    std::abort();
  try {
    captures(1);
    std::abort();
  } catch (int) {
  }
  if (dtors != 2) // neither the capture nor the local was constructed
    std::abort();
}
