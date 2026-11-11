// RUN: %clang -std=c++23 -fno-contracts-allow-assume -fno-contract-bypass-rethrowing-local-handler -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E178: the contracts option rule.  The negative forms of
// the contracts options that mean nothing with contracts off ask for
// nothing, so they do not warn (E178 (5)).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-negative.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
