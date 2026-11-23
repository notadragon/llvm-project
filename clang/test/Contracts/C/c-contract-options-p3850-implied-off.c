// RUN: %clang -std=c23 -fno-contracts-p3100 -fno-contracts-p4298 -fcontracts-p3850 -fno-contracts-p4405 -fsanitize-noncontract-callbacks -fcontracts-prove-fold -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s --implicit-check-not=warning: --implicit-check-not=error:

// DECISIONS.md E179 (1): on a C input -fcontracts-p3850 implies the C
// per-paper options except one explicitly disabled, whatever the order, and
// turns C contracts on itself even with all three disabled:
// -fsanitize-noncontract-callbacks and a P4405 strategy then warn that they
// have no effect, and C contracts stay on.  The driver half is
// c-contract-options-p3850-driver.c.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-p3850-implied-off.c

// CHECK-DAG: warning: '-fsanitize-noncontract-callbacks' has no effect without '-fcontracts-p3100'
// CHECK-DAG: warning: '-fcontracts-prove-fold' has no effect without '-fcontracts-p4405'

#ifndef __clang_contracts_p4299
#error "-fcontracts-p4299 is on"
#endif

int f(int x) _Pre(x > 0) { return x; }
