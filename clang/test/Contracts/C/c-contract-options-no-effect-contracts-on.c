// RUN: %clang -std=c23 -fcontracts-p4405 -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontracts-prove-fold -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -fcontracts-prove-assume -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule in C, where with C
// contracts on, here turned on by -fcontracts-p4405, the contracts options
// do not warn, nor the P4405 strategies with P4405 on.
// gnu_gcc's -fcontracts-client-check= and -fcontracts-definition-check=
// have no Clang counterpart.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-no-effect-contracts-on.c

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

int f(int x) _Pre(x > 0) { return x; }
