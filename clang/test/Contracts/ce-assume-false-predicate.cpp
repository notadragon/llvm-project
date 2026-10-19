// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-allow-assume -fcontract-evaluation-semantic=assume -fsyntax-only -verify %s
// expected-no-diagnostics

// Under the assume semantic, constant evaluation does not evaluate the
// predicate, so a false predicate neither makes the evaluation non-constant
// nor turns into a silent substitution failure in the concept below (the
// "ignore" reading, as in GCC and P3100's "potentially evaluated, but not
// evaluated").  The non-constant-predicate shape is
// ce-assume-nonconstant-predicate.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ce-assume-semantic.C

constexpr int f(int x) pre(x > 0) { return x; }

constexpr int a = f(-1);

template <int N> struct K {};
template <int V> concept CF = requires { typename K<f(V)>; };
static_assert(CF<-1>);
