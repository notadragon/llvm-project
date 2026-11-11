// RUN: %clang -std=c++23 -fno-contracts-allow-assume -fcontracts-allow-assume -fno-contract-bypass-rethrowing-local-handler -fcontract-bypass-rethrowing-local-handler -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s

// DECISIONS.md E175-E178: the contracts option rule.  A contracts option
// given negatively and then positively ends up in its positive form, which
// warns with contracts off (E178 (5)).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-positive-last.C

// CHECK-DAG: warning: '-fcontracts-allow-assume' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-bypass-rethrowing-local-handler' has no effect without '-fcontracts'
// CHECK-NOT: {{warning|error}}:

int i;
