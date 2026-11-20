// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s

// Discarding the result of a [[nodiscard]] virtual function with contracts
// is diagnosed under P3097.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/virtual-wrapper-nodiscard.C

struct B { [[nodiscard]] virtual int v(int x) pre(x > 0); };
void u(B &b) { b.v(1); } // expected-warning {{ignoring return value of function declared with 'nodiscard' attribute}}
