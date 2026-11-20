// RUN: not %clang -std=c++23 -fno-contracts -fcontracts-p3400 -fno-contracts-p4451 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  With -fno-contracts,
// -fcontracts-p3400 -fno-contracts-p4451 is diagnosed for -fcontracts-p3400
// given with -fno-contracts, not for -fno-contracts-p4451 taking part of
// P3400 away.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-p4451.C

// CHECK: error: '-fcontracts-p3400' cannot be combined with '-fno-contracts'
// CHECK-NOT: {{warning|error}}:

int i;
