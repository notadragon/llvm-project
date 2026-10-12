// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: not --crash %t

// P3098 x P3400 single-unit rule: under enforce the capture is constructed, the
// predicate is evaluated, and a failure terminates (the run is expected to fail).
// (GCC mirror: g++.dg/contracts/cpp26/p3098-enforce-capture.C)

#include <contracts>

static int made(int v) { return v; }

void handle_contract_violation(const std::contracts::contract_violation&) { }

int f() post [old = made(5)] (r: r == old) { return 7; }

int main() {
  return f();  // should terminate, not return
}
