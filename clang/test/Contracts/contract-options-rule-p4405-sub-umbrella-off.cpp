// RUN: %clang -std=c++26 -fcontracts-prove-fold -fcontracts-p3850 -fno-contracts-p4405 -fsyntax-only %s 2>&1 | FileCheck %s
// RUN: %clang -std=c++26 -fno-contracts-p4405 -fcontracts-p3850 -fcontracts-prove-fold -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  A P4405 sub-option
// given positively warns when P4405 ends up off, here through
// -fno-contracts-p4405 with the -fcontracts-p3850 umbrella, in either order.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-p4405-sub-umbrella-off.C

// CHECK: warning: '-fcontracts-prove-fold' has no effect without '-fcontracts-p4405'
// CHECK-NOT: {{warning|error}}:

int f(int x) pre(x > 0) { return x; }
