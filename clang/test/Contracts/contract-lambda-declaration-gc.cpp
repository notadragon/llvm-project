// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -emit-llvm -o /dev/null %s

// Lambdas, nested, in contracts on declarations in a class and at namespace
// scope: a lambda in a contract on a declaration is parsed with no enclosing
// function body.  It must compile.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-lambda-declaration-gc.C
// in the gnu_gcc fork (GCC-667, a garbage-collection use-after-free there;
// Clang has no collector), whose CLANG-647 row asked for it.

void f() { [] { return [] { return 1; }(); }(); }

struct S {
  int m(int y) pre([=] { return y > 0; }()) { return y; }
  int n(const int y) post(r: [=] { return r == y; }());
};

int g(int y) pre([=] { return [=] { return y > 0; }(); }());
int h(int y) pre([=] { return y > 0; }());

int use(S s) { return s.m(1) + s.n(1) + g(1) + h(1); }
