// RUN: %clang -std=c++23 -fcontracts-p3400 -fno-contracts-p3400 -fno-contracts -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule.  The final values
// decide: a per-paper option given positively and then negatively is off,
// so a final -fno-contracts is no error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-contracts-paper-off.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
