// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %s %libcxx_flags -o %t && %t

// The checks of lambdas, generic and not, in a function template.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-outlined-lambda-in-template.C
// in the gnu_gcc fork (GCC-657), whose CLANG-647 row asked for it.  GCC's test
// is under -fcontract-checks-outlined, which Clang does not have; the checks
// are inline here.

#include <contracts>

int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

template <class T> int t(T v) {
  auto l = [](T x) pre(x > 0) post(r: r == 1) { return 1; };
  auto g = [](auto x) pre(x > 0) { return 1; };
  return l(v) + g(v);
}

int main() {
  t(1);
  if (violations != 0)
    __builtin_abort();
  t(0);
  if (violations != 2)
    __builtin_abort();
}
