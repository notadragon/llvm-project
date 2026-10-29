// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.pointer.arithmetic:enforce
// expected-no-diagnostics

// P3100: with the pointer-wrap check configured, an address constant plus a
// constant offset is not guarded, so it stays a constant initializer.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-ptr-wrap-constinit.C)

int g[4];
constinit int *gp = g + 2;
constinit int *gq = &g[3] - 1;
