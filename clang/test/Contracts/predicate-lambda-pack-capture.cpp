// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// expected-no-diagnostics

// A lambda in a function template's precondition that captures the
// template's parameter pack, one in its postcondition capturing a const pack
// by reference, and the same in a contract_assert.  Run:
// Runnable/predicate-lambda-pack-capture.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/predicate-lambda-pack-capture.C
// in the gnu_gcc fork (GCC-656).

template <class... T>
int g(T... xs) pre([=] { return ((xs > 0) && ...); }()) { return 0; }
template <class... T>
int p(const T... xs) post([&] { return ((xs > 0) && ...); }()) { return 0; }
template <class... T>
int h(T... xs) { contract_assert([=] { return ((xs > 0) && ...); }()); return 0; }

int a = g(1, 2);
int b = h(1, 2);
int c = p(1, 2);
