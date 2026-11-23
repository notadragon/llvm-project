// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -fsyntax-only -verify %s
// expected-no-diagnostics

// Under the assume semantic, constant evaluation does not evaluate the
// predicate, so a non-constant predicate neither makes the evaluation
// non-constant nor turns into a silent substitution failure in the concept
// below.  The false-predicate shape is ce-assume-false-predicate.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ce-assume-semantic.C

int g(int x) { return x; }
constexpr int h(int x) pre(g(x) > 0) { return x; }

constexpr int b = h(1);

template <int N> struct K {};
template <int V> concept CH = requires { typename K<h(V)>; };
static_assert(CH<1>);
