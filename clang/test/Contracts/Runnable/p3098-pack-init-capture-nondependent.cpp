// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3098: an init-capture pack in a postcondition whose initializer names a
// pack but has a type that is not dependent (`...n = sizeof(args)`), in a
// function template (parsed with its declaration) and in a member function
// template (parsed when its class is complete).  Two of the four calls
// violate.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3098-pack-init-capture-nondependent.C
// (GCC-675, where this shape was rejected and then crashed).

#include <contracts>

static int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

template <class... T>
int g(T... args) post [...n = sizeof(args)] ((n + ... + 0) <= 8)
{ return 0; }

struct S {
  template <class... T>
  int f(T... args) post [...n = sizeof(args)] ((n + ... + 0) <= 8)
  { return 0; }
};

int main() {
  S s;
  g(1, 2);
  s.f(1, 2);
  if (violations != 0)
    __builtin_abort();
  g(1.0, 2.0);
  s.f(1.0, 2.0);
  if (violations != 2)
    __builtin_abort();
}
