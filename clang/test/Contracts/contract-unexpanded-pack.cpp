// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A parameter pack named unexpanded in a contract predicate is diagnosed in
// the template, as in any other expression.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-unexpanded-pack.C in
// the gnu_gcc fork (GCC-651 there, fixed).

template <class... T>
int f1(T... xs) pre(xs > 0) { return 0; } // expected-error {{expression contains unexpanded parameter pack 'xs'}}
template <class... T>
int f2(const T... xs) post(r: r > xs) { return 0; } // expected-error {{expression contains unexpanded parameter pack 'xs'}}
template <class... T>
int f3(T... xs) { contract_assert(xs > 0); return 0; } // expected-error {{expression contains unexpanded parameter pack 'xs'}}
template <class... T>
int f4(T... xs) pre(xs > 0); // expected-error {{expression contains unexpanded parameter pack 'xs'}}
template <class... T>
struct S { int m(T... xs) pre(xs > 0) { return 0; } }; // expected-error {{expression contains unexpanded parameter pack 'xs'}}

// Expanded, they are fine.
template <class... T>
int g1(const T... xs) pre(((xs > 0) && ...)) post(r: ((r > xs) || ...)) {
  contract_assert(((xs > 0) && ...));
  return 2;
}
template <class... T>
int g2(T... xs) { contract_assert([=] { return ((xs > 0) && ...); }()); return 0; }

int main() { return g1(1, 1) + g2(1, 2) - 2; }
