// RUN: not %clang -std=c23 -fcontracts-p3100 -fcontracts-p4298 -fcontracts-p4405 -fno-contracts-p4299 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule in C, where
// -fcontracts-p3100, -fcontracts-p4298 and -fcontracts-p4405 given
// positively are errors when a final -fno-contracts-p4299 turns C contracts
// off; the order does not matter (c-contract-options-p4299-off-before.c).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4299-off-after.c

// CHECK: error: '-fcontracts-p3100' cannot be combined with '-fno-contracts-p4299'
// CHECK-NEXT: error: '-fcontracts-p4298' cannot be combined with '-fno-contracts-p4299'
// CHECK-NEXT: error: '-fcontracts-p4405' cannot be combined with '-fno-contracts-p4299'
// CHECK-NOT: {{warning|error}}:

int i;
