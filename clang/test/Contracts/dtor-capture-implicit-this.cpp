// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// A destructor's postcondition capture initializer runs at entry, while the
// object is alive, so the implicit (*this) transformation is valid there
// (P3098 [expr.prim.id.general] forbids it only in a destructor's
// postcondition predicate).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/cdtor-capture-implicit-this-dtor.C

struct A2 {
  int v = 0;
  ~A2() post [c = v] (c == 0) {}
};
