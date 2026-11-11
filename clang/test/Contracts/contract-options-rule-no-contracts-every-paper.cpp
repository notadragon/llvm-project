// RUN: not %clang -std=c++23 -fcontracts-p3097 -fcontracts-p3098 -fcontracts-p3099 -fcontracts-p3100 -fcontracts-p3290 -fcontracts-p3400 -fcontracts-p3850 -fcontracts-p4283 -fcontracts-p4298 -fcontracts-p4301 -fcontracts-p4405 -fcontracts-p4451 -fno-contracts -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule.  Every per-paper option
// given positively is diagnosed with a final -fno-contracts, each once,
// including -fcontracts-p3100 (which would otherwise route the sanitizers)
// and the -fcontracts-p3850 umbrella.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-every-paper.C

// CHECK: error: '-fcontracts-p3097' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p3098' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p3099' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p3100' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p3290' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p3400' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p3850' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p4283' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p4298' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p4301' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p4405' cannot be combined with '-fno-contracts'
// CHECK-NEXT: error: '-fcontracts-p4451' cannot be combined with '-fno-contracts'
// CHECK-NOT: {{warning|error}}:

int i;
