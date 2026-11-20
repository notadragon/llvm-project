// RUN: %clang -std=c23 -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=ignore -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175-E178: the contracts option rule in C, where a joined
// contracts option warns without C contracts for any value, its default
// included (E178 (5)).
// gnu_gcc's -fcontracts-client-check= and -fcontracts-definition-check=
// have no Clang counterpart.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-no-effect-default-value.c

// CHECK-DAG: warning: '-fcontract-evaluation-semantic=' has no effect without '-fcontracts-p4299'
// CHECK-DAG: warning: '-fcontract-analysis-semantic=' has no effect without '-fcontracts-p4299'
// CHECK-NOT: {{warning|error}}:

int i;
