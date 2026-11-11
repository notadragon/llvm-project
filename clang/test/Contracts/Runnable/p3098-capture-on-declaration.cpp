// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3098 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// A postcondition capture (P3098) on a declaration that is not the
// definition is initialized from the parameter the call passes, through the
// definition (whose parameter has another name here): on a declaration
// before the definition, on a member function declared in its class, and on
// a function template.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3098-capture-on-declaration-run.C
// in the gnu_gcc fork (GCC-658).

#include <contracts>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int g(int a) post [old = a] (old > 0);
int g(int y) { return y; }

struct S {
  int m(int a) post [old = a] (old > 0);
};
int S::m(int y) { return y; }

template <class T> T t(T a) post [old = a] (old > 0);
template <class T> T t(T y) { return y; }

int main() {
  g(1);
  S{}.m(1);
  t(1);
  if (violations != 0)
    return 1;
  g(0);
  if (violations != 1)
    return 2;
  S{}.m(0);
  if (violations != 2)
    return 3;
  t(0);
  if (violations != 3)
    return 4;
  return 0;
}
