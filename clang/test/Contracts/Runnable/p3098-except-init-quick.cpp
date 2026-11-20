// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t
// RUN: not --crash %t

// P3098: an exception during capture initialization under quick_enforce
// terminates the program without calling the handler.  A handler call, or
// the exception escaping f, is a failure, reported by exiting normally.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-except-init-quick.C

#include <contracts>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::_Exit(0);
}

struct Bad {
  Bad(int) { throw 42; }
  ~Bad() {}
};

int f(int i) post [b = Bad(1)] (true) { return i; }

int main() {
  try {
    f(10);
  } catch (...) {
    std::_Exit(0);
  }
  return 0;
}
