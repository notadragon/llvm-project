// RUN: not %clangxx -std=c++26 %s -fcontracts -fcontracts-p4283 %libcxx_flags \
// RUN:   -fsyntax-only 2>&1 | FileCheck %s

// a malformed requires-clause on a contract of a templated function is
// diagnosed -- here a requires-clause with no following parenthesized contract
// condition.  (The non-templated-function error is covered by p4283-errors.cpp.)
// (GCC mirror: g++.dg/contracts/cpp26/p4283-malformed.C, adapted to Clang's
// diagnostic: Clang reports the missing contract condition as "expected '('
// after 'pre'".)

// CHECK: error: expected '(' after 'pre'
template <class T> int a(T x) pre requires (true) ;
