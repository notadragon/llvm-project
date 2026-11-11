// RUN: %clang -std=c23 -fno-contracts-p4299 -fno-contracts-p3100 -fno-contracts-p4298 -fno-contracts-p4405 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule in C, where negative
// per-paper options are always allowed, also with -fno-contracts-p4299.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4299-off-negative.c

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __clang_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
