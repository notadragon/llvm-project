// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce
// expected-no-diagnostics

// P3100: with the pointer-wrap check configured, pointer arithmetic is still a
// constant expression: the check is code generation only.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-constexpr.C)

constexpr int f(int i) { int a[4]{1, 2, 3, 4}; int *p = a; p = p + i; return *p; }
static_assert(f(2) == 3);
constexpr int g(int i) { int a[4]{}; int *p = a + 4; p -= i; return p - a; }
static_assert(g(4) == 0);
