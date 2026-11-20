// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t && %t 2>&1 | FileCheck %s

// The other half of [basic.contract.eval]/17's note: "If the function has a
// non-throwing exception specification, the function std::terminate is
// invoked ([except.terminate])."
//
// The handler throws out of a postcondition on a noexcept function.  Since
// the behavior is as if the function body exits via that exception, and the
// function is noexcept, the program terminates -- the try/catch in the body
// does not run, and neither does the one in main.  A terminate handler says
// which happened, and each catch exits with its own status, so the run tells
// terminate apart from either catch.
//
// GCC mirror: g++.dg/contracts/cpp26/postcondition-throw-noexcept-terminate.C

#include <contracts>
#include <cstdio>
#include <cstdlib>
#include <exception>

struct Bail {};

void handle_contract_violation(const std::contracts::contract_violation &) {
  throw Bail{};
}

static int f() noexcept post(r : r > 0) {
  try {
    return -1;
  } catch (...) {
    std::fputs("caught in the body\n", stderr);
    std::_Exit(3); // must not run
  }
}

int main() {
  std::set_terminate([] {
    std::fputs("terminated\n", stderr);
    std::_Exit(0);
  });
  try {
    f();
  } catch (const Bail &) {
    std::fputs("caught in main\n", stderr);
    std::_Exit(4); // must not run either
  }
  return 5;
}

// CHECK-NOT: caught
// CHECK: terminated
