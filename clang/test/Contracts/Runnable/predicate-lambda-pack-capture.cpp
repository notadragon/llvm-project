// RUN: %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A lambda in a function template's precondition or postcondition captures
// the template's function parameter pack, by copy and by reference, and
// checks every element at run time, as the same lambda in a contract_assert
// in the body does.  The compile-only shape is
// ../predicate-lambda-pack-capture.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/predicate-lambda-pack-capture.C
// in the gnu_gcc fork (GCC-656).

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

template <class... T>
int g(T... xs) pre([=] { return ((xs > 0) && ...); }()) { return 0; }
template <class... T>
int p(const T... xs) post([&] { return ((xs > 0) && ...); }()) { return 0; }
template <class... T>
int h(T... xs) { contract_assert([=] { return ((xs > 0) && ...); }()); return 0; }

int main() {
  g(1, 2);
  p(1, 2);
  h(1, 2);
  if (violations != 0)
    return 1;
  g(1, -2);
  if (violations != 1)
    return 2;
  p(-1, 2);
  if (violations != 2)
    return 3;
  h(1, -2);
  if (violations != 3)
    return 4;
  return 0;
}
