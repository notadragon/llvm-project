// RUN: %clang -std=c23 -fcontracts-allow-assume -fcontracts-p4299 -fno-contracts-p4299 -fcontract-group-evaluation-semantic=g:observe -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule in C, where the
// contracts options warn that they have no effect when an explicit
// -fno-contracts-p4299 turns C contracts off, whatever the order, the
// configuration options with their own warning.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-no-effect-p4299-off.c

// CHECK-DAG: warning: '-fcontracts-allow-assume' has no effect without '-fcontracts-p4299'
// CHECK-DAG: warning: contract configuration options have no effect without '-fcontracts-p4299'
// CHECK-NOT: {{warning|error}}:

int i;
