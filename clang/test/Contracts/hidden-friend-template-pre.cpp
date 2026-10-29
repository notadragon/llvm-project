// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// A hidden friend defined in a class template with a precondition naming a
// member through a parameter, or a parameter only: instantiated and checked.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/hidden-friend-template-pre.C
// and hidden-friend-template-pre-param.C.

#include <contracts>

static int handled = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handled;
}

template <class T> struct TS {
  friend void tg(TS s) pre(s.m > 0) {}
  T m;
};

template <class T> struct S {
  friend void fr2(S, T t) pre(t > 0) {}
};

int main() {
  tg(TS<int>{1});
  fr2(S<int>{}, 1);
  if (handled != 0)
    __builtin_abort();
  tg(TS<int>{0});
  fr2(S<int>{}, 0);
  if (handled != 2)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
