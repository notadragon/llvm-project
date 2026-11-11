// RUN: %clang -std=c++23 -fno-contracts-prove-fold -fno-contract-analysis-skip-dead-calls -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule.  No warning for a P4405
// sub-option given negatively with contracts off.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-p4405-sub-negative.C

// CHECK-NOT: {{warning|error}}:

int i;
