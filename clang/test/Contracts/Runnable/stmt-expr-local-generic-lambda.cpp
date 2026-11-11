// RUN: %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A variable declared in a GNU statement expression in a generic lambda's
// precondition or postcondition is checked at run time, for a postcondition
// with a trailing return type and with a deduced one.  The compile-only shape
// is ../stmt-expr-local-generic-lambda.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-generic-lambda.C
// in the gnu_gcc fork (GCC-652); the deduced return type is GCC's GCC-660,
// deferred (gcc/testsuite/g++.dg/contracts/cpp26/open-bug-stmt-expr-local-generic-lambda-deduced-post.C).

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int k(int v) {
  auto m = [](auto x) pre(({ int y = x; y > 0; })) { return x; };
  return m(v);
}

int k2(int v) {
  auto m = [](auto x) -> int post(r: ({ int z = r; z > 0; })) { return x; };
  return m(v);
}

int k3(int v) {
  auto m = [](auto x) post(r: ({ int z = r; z > 0; })) { return x; };
  return m(v);
}

int main() {
  k(1);
  k2(1);
  k3(1);
  if (violations != 0)
    return 1;
  k(-1);
  if (violations != 1)
    return 2;
  k2(-1);
  if (violations != 2)
    return 3;
  k3(-1);
  if (violations != 3)
    return 4;
  return 0;
}
