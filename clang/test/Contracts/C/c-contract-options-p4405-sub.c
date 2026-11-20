// RUN: %clang -std=c23 -fcontracts-p4299 -fcontracts-prove-fold -fno-contracts-prove-shadow -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule in C, where a P4405
// strategy given positively with C contracts on but P4405 off warns that it
// has no effect; one given negatively does not.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p4405-sub.c

// CHECK: warning: '-fcontracts-prove-fold' has no effect without '-fcontracts-p4405'
// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

int f(int x) _Pre(x > 0) { return x; }
