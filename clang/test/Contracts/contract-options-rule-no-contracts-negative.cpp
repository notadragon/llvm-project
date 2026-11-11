// RUN: %clang -std=c++26 -fno-contracts -fno-contracts-p3400 -fno-contracts-p3850 -fno-contracts-p4451 -fno-contracts-p3100 -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule.  Negative per-paper
// options are always allowed, also with -fno-contracts, and are not reported
// as unused.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-negative.C

// CHECK-NOT: {{warning|error}}:

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int f(int x) pre(x > 0) { return x; } // expected-error {{expected function body after function declarator}}
