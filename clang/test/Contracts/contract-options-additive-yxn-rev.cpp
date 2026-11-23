// RUN: not %clang -std=c++23 -fno-contracts-p4451 -fcontracts-p3850 -fsyntax-only %s 2>&1 | FileCheck %s

// DECISIONS.md E175: -fcontracts-p3850, -fcontracts-p3400 and
// -fcontracts-p4451 are additive, whatever their order.  Columns of the name:
// p3850, p3400, p4451; y given, n given as -fno-, x not given; -rev: the -fno- options first.
// Expected: an error, since P3400 is on and -fno-contracts-p4451 would
// take away part of it.
// -std=c++23, so that only the options can turn contracts on.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-yxn-rev.C

// CHECK: error: '-fno-contracts-p4451' cannot be combined with '-fcontracts-p3850', which implies '-fcontracts-p3400'
// CHECK-NOT: error:

int i;
