// RUN: not %clang -std=c++23 -fno-contracts -fcontracts-p4451 -fcontracts-p3850 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175 and E176: the per-paper options are additive, but an
// explicit -fno-contracts still wins over every one of them, whatever the
// order, and each given positively is then an error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-no-contracts.C

// CHECK-DAG: error: '-fcontracts-p4451' cannot be combined with '-fno-contracts'
// CHECK-DAG: error: '-fcontracts-p3850' cannot be combined with '-fno-contracts'
// CHECK-NOT: argument unused

int i;
