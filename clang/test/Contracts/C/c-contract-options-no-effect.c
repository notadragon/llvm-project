// RUN: %clang -std=c23 -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontracts-prove-fold -fcontracts-prove-shadow -fcontracts-prove-override -fcontracts-prove-callee-premises -fcontracts-prove-assume -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck %s

// DECISIONS.md E175-E177: the contracts option rule in C, where the
// contracts options that mean nothing without C contracts warn that they
// have no effect without -fcontracts-p4299, as do the P4405 strategies given
// positively; a joined option is named without its argument.
// gnu_gcc's -fcontracts-client-check= and -fcontracts-definition-check=
// have no Clang counterpart.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-no-effect.c

// CHECK: warning: '-fcontracts-allow-assume' has no effect without '-fcontracts-p4299'
// CHECK-NEXT: warning: '-fcontract-evaluation-semantic=' has no effect without '-fcontracts-p4299'
// CHECK-NEXT: warning: '-fcontract-analysis-semantic=' has no effect without '-fcontracts-p4299'
// CHECK-NEXT: warning: '-fcontracts-prove-fold' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-shadow' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-override' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-callee-premises' has no effect without '-fcontracts-p4405'
// CHECK-NEXT: warning: '-fcontracts-prove-assume' has no effect without '-fcontracts-p4405'
// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __clang_contracts_p4299
#error "-fcontracts-p4299 is off"
#endif

int i;
