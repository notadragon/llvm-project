// RUN: %clang -std=c++23 -fcontracts-allow-assume -fno-contracts-allow-assume -fcontract-bypass-rethrowing-local-handler -fno-contract-bypass-rethrowing-local-handler -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E178: the contracts option rule.  A contracts option
// given positively and then negatively ends up in its negative form, which
// does not warn (E178 (5)), nor is it reported as unused.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-negative-last.C

// CHECK-NOT: {{warning|error}}:

int i;
