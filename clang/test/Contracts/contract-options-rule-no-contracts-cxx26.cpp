// RUN: not %clang -std=c++26 -fno-contracts -fcontracts-p3098 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  In C++26, where the
// standard turns contracts on, an explicit -fno-contracts still turns them
// off, and a per-paper option given positively with it is an error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-cxx26.C

// CHECK: error: '-fcontracts-p3098' cannot be combined with '-fno-contracts'
// CHECK-NOT: {{warning|error}}:

int i;
