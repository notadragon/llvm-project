// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A result binding is a local entity ([basic.pre]) and is captured by a lambda
// in the postcondition that names it, implicitly by reference or by copy, in
// nested lambdas, in a function template, for a reference result and in a
// generic lambda (in a member function; a free function's generic lambda is
// CLANG-605).  At run time the lambda reads the returned object, and in
// constant evaluation the call is a constant expression.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-binding-lambda-capture.C

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
}

int f1() post(r: [&] { return r; }() == 5) { return 5; }
int f2() post(r: [=] { return r; }() == 5) { return 5; }
int f3() post(r: [&] { return r; }() == 6) { return 5; } // violation
int f4() post(r: [&] { return [&] { return r; }(); }() == 5) { return 5; }
struct S {
  int f5() post(r: [&](auto k) { return r + k; }(1) == 6) { return 5; }
};

template <class T> T t1(const T x) post(r: [&] { return r; }() == x) { return x; }

int g = 7;
int &f6() post(r: [&] { return &r; }() == &g) { return g; }

constexpr int c1() post(r: [&] { return r; }() == 5) { return 5; }
static_assert(c1() == 5);

int main() {
  f1();
  f2();
  f4();
  S{}.f5();
  t1(3);
  f6();
  if (handled != 0)
    __builtin_abort();
  f3();
  if (handled != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
