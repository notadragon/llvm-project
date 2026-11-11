// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A templated function first declared with a precondition whose lambda
// captures a parameter, and defined later without repeating the contract
// (which is allowed), checks that contract once instantiated.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-omit-contract-lambda.C

#include <contracts>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

template <class T> struct S { int f(int x) pre([&] { return x > 0; }()); };
template <class T> int S<T>::f(int x) { return x; }

struct A { template <class T> T g(T x) pre([&] { return x > 0; }()); };
template <class T> T A::g(T x) { return x; }

template <class T> T fa(T x) pre([&] { return x > 0; }());
template <class T> T fa(T x) { return x; }

int main() {
  S<int>{}.f(1);
  A{}.g(1);
  fa(1);
  if (reported != 0)
    __builtin_abort();
  S<int>{}.f(-1);
  A{}.g(-1);
  fa(-1);
  if (reported != 3)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
