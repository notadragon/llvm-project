// RUN: %clang -std=c++26 -fcontracts -fcontracts-prove-fold -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -fcontracts-prove-assume -fcontract-analysis-skip-dead-calls -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  The P4405 sub-options
// given positively without -fcontracts-p4405 warn that they have no effect.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-p4405-sub.C

// CHECK: warning: '-fcontracts-prove-fold' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-shadow' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-override' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-callee-premises' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-assume' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontract-analysis-skip-dead-calls' has no effect without '-fcontracts-p4405'
// CHECK-NOT: {{warning|error}}:

int f(int x) pre(x > 0) { return x; }
