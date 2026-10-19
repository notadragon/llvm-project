// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -ast-print %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -ast-print %s > %t.cpp
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only %t.cpp

// -ast-print puts the function contract specifiers after the virt-specifiers
// (override, final), where they parse back.
//
// No GCC mirror: the AST printer has no GCC counterpart.

struct B { virtual int f(int x) const pre(x > 0) = 0; };
struct D : B {
  int f(int x) const override final pre(x > 0);
};

// CHECK: int f(int x) const override final pre(x > 0);
