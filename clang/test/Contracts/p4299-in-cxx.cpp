// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// -fcontracts-p4299 in a C++ compile makes the C contract spellings _Pre,
// _Post and _ContractAssert available, with the meaning of pre, post and
// contract_assert: each is checked.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4299-in-cxx.C

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int f(int x) _Pre(x > 0) _Post(r : r > 0) {
  _ContractAssert(x > 0);
  return x;
}

int main() {
  f(1);
  if (violations != 0)
    __builtin_abort();
  f(-1); // _Pre, _ContractAssert and _Post
  if (violations != 3)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
