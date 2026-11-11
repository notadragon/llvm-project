// RUN: %clang -std=c++26 -fno-contracts -fcontract-evaluation-semantic=enforce -fcontract-analysis-semantic=ignore -fcontract-bypass-rethrowing-local-handler -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s

// DECISIONS.md E175-E178: the contracts option rule.  A joined contracts
// option warns with contracts off for any value, its default included
// (E178 (5)), as does a positive boolean option whose default is on.
// gnu_gcc's -fcontracts-client-check=, -fcontracts-definition-check=,
// -fcontract-checks-outlined, -fcontract-disable-optimized-checks and
// -fcontract-comment-from-expression have no Clang counterpart.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-default-value.C

// CHECK-DAG: warning: '-fcontract-evaluation-semantic=' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-analysis-semantic=' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-bypass-rethrowing-local-handler' has no effect without '-fcontracts'
// CHECK-NOT: {{warning|error}}:

int i;
