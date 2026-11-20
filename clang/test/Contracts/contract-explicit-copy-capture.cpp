// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// In a contract predicate an entity declared outside it is named with const
// type, but a by-copy capture of it is a closure member of the entity's own
// type, so a mutable lambda may modify it ([expr.prim.id.unqual] example).
// The result-binding shape is in result-binding-lambda-explicit-capture.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-explicit-copy-capture.C

void a1() {
  int v = 1;
  contract_assert([v]() mutable { return ++v; }() > 0);
}

int h1(int x) pre([x]() mutable { return ++x; }() > 0) { return x; }

template <class T>
T t1(T x) pre([x]() mutable { return ++x; }() > 0) { return x; }

int u = t1(1);
