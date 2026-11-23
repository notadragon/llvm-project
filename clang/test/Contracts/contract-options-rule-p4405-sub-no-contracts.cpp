// RUN: %clang -std=c++23 -fcontracts-prove-shadow -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  A P4405 sub-option
// given positively warns when contracts end up off (P4405 then is off too).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-p4405-sub-no-contracts.C

// CHECK: warning: '-fcontracts-prove-shadow' has no effect without '-fcontracts-p4405'
// CHECK-NOT: {{warning|error}}:

int i;
