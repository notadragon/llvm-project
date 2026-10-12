// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm-only -verify %s
// expected-no-diagnostics

// A variable declared in a GNU statement expression in the postcondition of
// a lambda with a deduced return type, initialized from the result name: the
// predicate is rebuilt once the return type is deduced, and the variable's
// type and initializer are rebuilt with it, whatever the form of the
// initialization.  The explicit return type and the precondition are in
// stmt-expr-local-lambda-pre.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/stmt-expr-local-lambda-deduced-post.C

void f() {
  auto l1 = [](int x) post(r: ({ int z = r; z > 0; })) { return x; };
  auto l2 = [](int x) post(r: ({ auto z = r; z > 0; })) { return x; };
  auto l3 = [](int x) post(r: ({ int z(r); int w{r}; z + w > 0; })) { return x; };
  auto l4 = [](int x) pre(({ int y = x; y > 0; })) post(r: ({ int z = r; z > 0; })) { return x; };
  l1(1);
  l2(1);
  l3(1);
  l4(1);
}
