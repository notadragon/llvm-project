// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// A lambda in the contract of a function declaration (not a definition), in
// a class or at namespace scope, followed by a declaration with a default
// argument.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/decl-contract-lambda-default-arg.C
// and decl-contract-lambda-default-arg-nsscope.C

struct S {
  void f0(int x) pre([] { return true; }());
};

void f1(int x) pre([x] { return x > 0; }());

void f4(int y = 1);
