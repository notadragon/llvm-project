// RUN: not %clang -std=c++23 -fno-contracts -fcontracts-p3400 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  A per-paper option
// given positively is an error when a final -fno-contracts turns contracts
// off; the order does not matter (contract-options-rule-no-contracts-after.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-before.C

// CHECK: error: '-fcontracts-p3400' cannot be combined with '-fno-contracts'
// CHECK-NOT: {{warning|error}}:

int i;
