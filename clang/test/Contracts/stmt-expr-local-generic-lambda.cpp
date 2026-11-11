// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// expected-no-diagnostics

// A variable declared in a GNU statement expression in a generic lambda's
// precondition or postcondition, with a deduced return type (k2) and a
// trailing one (k3).  Run: Runnable/stmt-expr-local-generic-lambda.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-generic-lambda.C
// in the gnu_gcc fork (GCC-652), and for k2
// gcc/testsuite/g++.dg/contracts/cpp26/open-bug-stmt-expr-local-generic-lambda-deduced-post.C,
// the watch test for GCC's GCC-660 (GCC ICEs; deferred).

int k() {
  auto m = [](auto x) pre(({ int y = x; y > 0; })) { return x; };
  return m(1);
}

int k2() {
  auto m = [](auto x) post(r: ({ int z = r; z > 0; })) { return x; };
  return m(1);
}

int k3() {
  auto m = [](auto x) -> int post(r: ({ int z = r; z > 0; })) { return x; };
  return m(1);
}
