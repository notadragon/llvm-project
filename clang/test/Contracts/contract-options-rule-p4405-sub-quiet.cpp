// RUN: %clang -std=c++26 -fno-contracts-prove-fold -fno-contract-analysis-skip-dead-calls -fcontracts-prove-shadow -fcontracts-p4405 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule.  No warning for a P4405
// sub-option given negatively without P4405, nor for one given positively
// before -fcontracts-p4405.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-p4405-sub-quiet.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

int f(int x) pre(x > 0) { return x; }
