// RUN: %clang -std=c++26 -fno-contracts-allow-assume -fno-contracts -fno-contract-bypass-rethrowing-local-handler -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E178: the contracts option rule.  The negative forms of
// the contracts options do not warn when an explicit -fno-contracts turns
// contracts off either, whether given before or after it (E178 (5)).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-negative-no-contracts.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
