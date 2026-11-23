// RUN: %clang -std=c++23 -fno-contracts -fcontracts-p3400 -fcontracts -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule.  The last of
// -fcontracts and -fno-contracts decides: a -fno-contracts followed by
// -fcontracts leaves contracts on, and the per-paper option given between
// them is no error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-contracts-last.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifndef __cpp_contracts_labels
#error "P3400 is on"
#endif

int f(int x) pre(x > 0) { return x; }
