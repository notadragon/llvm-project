// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// expected-no-diagnostics

// A variable declared in a GNU statement expression in a lambda's
// precondition, and in the postcondition of a lambda with an explicit return
// type.  Mirror: gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-lambda-pre.C.
// (A deduced return type is stmt-expr-local-lambda-deduced-post.cpp.)

void f() {
  auto l1 = [](const int x) pre(({ int y = x; y > 0; })) {};
  auto l2 = [](int x) -> int post(r: ({ int z = r; z > 0; })) { return x; };
  l1(1);
  l2(1);
}
