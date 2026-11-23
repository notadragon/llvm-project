// RUN: not %clang -std=c23 -fcontracts-p3850 -fno-contracts-p4299 -fsyntax-only %s 2>&1 | FileCheck %s
// RUN: not %clang -std=c23 -fno-contracts-p4299 -fcontracts-p3850 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E179 (1): -fcontracts-p3850 on a C input turns C contracts
// on, so it is an error with a final -fno-contracts-p4299, whatever the
// order, as the other C per-paper options are
// (c-contract-options-p4299-off-after.c).

// CHECK: error: '-fcontracts-p3850' cannot be combined with '-fno-contracts-p4299'
// CHECK-NOT: {{warning|error}}:

int i;
