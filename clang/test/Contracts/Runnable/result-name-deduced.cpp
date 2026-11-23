// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A postcondition result name on a function with a deduced return type is
// checked at run time against the deduced result: a function, a member, a
// function template, a lambda naming a capture and a generic lambda.  The
// predicate never sees a declaration of the same name in the body (hidden,
// lam).  (EDG-9 shapes.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-deduced-run.C;
// the constant-evaluation half is ../result-name-deduced.cpp.

#include <contracts>
#include <cstdlib>

static int n = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++n;
}

int gl = 1;
auto f(const int x) post(r : r == x) { return x; }         // OK
auto g(const int x) post(r : r == x) { return x + 1; }     // violation
auto hidden() post(r : r == gl) { int gl = 2; return gl; } // violation
template <class T> auto t(T x) post(r : r > 0) { return x; }
struct S {
  auto m(const int x) post(r : r == x) { return x; } // OK
};

int main() {
  f(1);
  g(1);
  hidden();
  t(1);  // OK
  t(-1); // violation
  S{}.m(2);
  int k = 1;
  auto lam = [k](int x) post(r : r == k) { int k = 5; return k + x; };
  lam(0); // violation
  auto gen = [](const auto x) post(r : r == x) { return x + 1; };
  gen(1); // violation
  if (n != 5)
    std::abort();
}
