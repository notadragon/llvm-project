// RUN: not %clang -std=c++26 -fno-contracts -fcontracts-p3850 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  With -fno-contracts,
// only the options actually given are diagnosed: -fcontracts-p3850, not each
// per-paper option it implies, nor -fcontracts-p4451, which
// -fcontracts-p3850 and -fcontracts-p3400 imply.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-umbrella.C

// CHECK: error: '-fcontracts-p3850' cannot be combined with '-fno-contracts'
// CHECK-NOT: {{warning|error}}:

int i;
