// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce %libcxx_flags -o %t && %t

// labels::review maps noexcept_enforce, a terminating semantic, to
// noexcept_observe (it keeps the nothrow property), as it maps enforce and
// quick_enforce to observe: the violation is observed and the program
// continues.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/review-noexcept-enforce.C

#include <contracts>

static int calls = 0;
static bool terminating = true;
static bool nothrow_observe = false;
void handle_contract_violation(const std::contracts::contract_violation &v) {
  ++calls;
  terminating = v.is_terminating();
  nothrow_observe =
      v.semantic() == std::contracts::evaluation_semantic::noexcept_observe;
}

int f(int x) pre<std::contracts::labels::review>(x > 0) { return x; }

int main() {
  f(0);
  if (calls != 1 || terminating || !nothrow_observe)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
