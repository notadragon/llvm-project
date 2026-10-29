// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4283 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s
// expected-no-diagnostics

// A lambda in a function template whose only contract specifier is
// discarded in an instance (its requires-clause is not satisfied) has no
// contract assertion there, so l1(-1.5) is a constant 1; the same when all
// of several are discarded.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4283-lambda-only-contract-discarded.C

template <class T> concept Int = __is_same(T, int);

template <class T> constexpr int l1(T t) {
  auto a = [](T x) pre requires Int<T> (x > 0) { return 1; };
  return a(t);
}
static_assert(l1(-1.5) == 1);
static_assert(l1(1) == 1);

template <class T> constexpr int l2(T t) {
  auto a = [](T x) pre requires Int<T> (x > 0) post requires Int<T> (r : r > 0) { return 1; };
  return a(t);
}
static_assert(l2(-1.5) == 1);
static_assert(l2(1) == 1);
