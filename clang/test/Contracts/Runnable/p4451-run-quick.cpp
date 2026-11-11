// DEFINE: %{run} = %clangxx -std=c++26 %s -fcontract-evaluation-semantic=quick_enforce %libcxx_flags -o %t
//
// RUN: %{run} -fcontracts-p4451 && not --crash %t
// RUN: %{run} -fcontracts-p3400 && not --crash %t
// RUN: %{run} -fcontracts-p3400 -fcontracts-p4451 && not --crash %t

// P4451: a configured quick_enforce is a terminating semantic and is kept
// for a terminating contract assertion: the violation traps without calling
// the handler, which would exit successfully.  Under -fcontracts-p4451
// alone, -fcontracts-p3400 alone and both.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-run-quick.C (and its
// -p3400 and -both variants)

#include <contracts>  // `contract_violation`, `terminating`
#include <cstdlib>    // `std::_Exit`

void handle_contract_violation(const std::contracts::contract_violation &) {
  std::_Exit(0);
}

int f(int x) pre<terminating>(x > 0) { return x; }

int main() { return f(-1); }
