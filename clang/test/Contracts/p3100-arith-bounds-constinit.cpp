// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -std=c++26 -fcontracts \
// RUN:   -fcontracts-p3100 -emit-llvm -o - %s \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.add.out.of.bounds.known.arithmetic:enforce \
// RUN:   | FileCheck %s
// expected-no-diagnostics

// P3100: with the known-bound pointer-arithmetic check configured, a constant
// offset in range is not guarded, so an address constant stays a constant
// initializer, and a constant in-range offset at run time emits no check.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-arith-bounds-constinit.C)

int g[4];
constinit int *gp = g + 2;
constinit int *ge = g + 4;

// CHECK-LABEL: define{{.*}} @_Z4fromv(
// CHECK-NOT: __cxa_contract_violation
// CHECK: ret ptr
int *from() { int *volatile q = g; (void)q; return g + 3; }
