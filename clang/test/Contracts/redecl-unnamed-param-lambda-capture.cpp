// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A contract on a first declaration whose lambda captures a parameter, with
// a definition that leaves that parameter unnamed (allowed).  The contract
// is rebuilt against the definition's parameters, and capturing the unnamed
// one must not be rejected as the capture of an unnamed variable.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/redecl-unnamed-param-lambda-capture.C

#include <contracts>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

int f(int x) pre([&] { return x > 0; }());
int f(int) { return 0; }

int g(int x) pre([=] { return x > 0; }());
int g(int) { return 0; }

struct B { int h(int x) pre([&] { return x > 0; }()); };
int B::h(int) { return 0; }

template <class T> struct S { int f(int x) pre([&] { return x > 0; }()); };
template <class T> int S<T>::f(int) { return 0; }

template <class T> T fa(T x) pre([&] { return x > 0; }());
template <class T> T fa(T) { return 0; }

int main() {
  f(1);
  g(1);
  B{}.h(1);
  S<int>{}.f(1);
  fa(1);
  if (reported != 0)
    __builtin_abort();
  f(-1);
  g(-1);
  B{}.h(-1);
  S<int>{}.f(-1);
  fa(-1);
  if (reported != 5)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
