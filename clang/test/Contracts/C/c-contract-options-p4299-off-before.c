// RUN: not %clang -std=c23 -fno-contracts-p4299 -fcontracts-p3100 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule in C, where a per-paper
// option given positively is an error when a final -fno-contracts-p4299
// turns C contracts off, whatever the order
// (c-contract-options-p4299-off-after.c).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4299-off-before.c

// CHECK: error: '-fcontracts-p3100' cannot be combined with '-fno-contracts-p4299'
// CHECK-NOT: {{warning|error}}:

int i;
