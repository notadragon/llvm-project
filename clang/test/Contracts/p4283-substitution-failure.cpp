// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fsyntax-only -verify %s
// expected-no-diagnostics

// A substitution failure in a contract's requires-clause (P4283) means the
// constraint is not satisfied, and the contract is discarded; a conjunction
// whose first operand is not satisfied does not substitute the second, nor a
// disjunction whose first operand is satisfied.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-substitution-failure.C

template <class T>
constexpr int f(T t) pre requires(T::value > 0)(t < T::value) { return 1; }

template <class T> concept C = sizeof(T) > 100;
template <class T>
constexpr int g(T t) pre requires(C<T> && T::value > 0)(t > 0) { return 1; }

template <class T>
constexpr int k(T t) pre requires(!C<T> || T::value > 0)(t > 0) { return 1; }

template <class T>
constexpr int m(T t) {
  contract_assert requires(T::value > 0)(t < T::value);
  return 1;
}

constexpr int n() { return f(1) + g(1) + k(1) + m(1); }
static_assert(n() == 4);
