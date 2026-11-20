// RUN: %clang -std=c++26 -fcontracts-allow-assume -fno-contracts -fcontract-evaluation-semantic=observe -fcontract-group-evaluation-semantic=g:observe -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck %s
// RUN: %clang -std=c++26 -fno-contracts -fcontract-configuration='[]' -fcontract-configuration-file=%S/no-such-file.json -fsyntax-only %s 2>&1 \
// RUN:   | FileCheck --check-prefix=CONFIG %s

// DECISIONS.md E175-E177: the contracts option rule.  The contracts options
// that mean nothing with contracts off warn that they have no effect when an
// explicit -fno-contracts turns contracts off, the configuration options
// with their own warning, once, whatever the order.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-no-contracts.C

// CHECK-DAG: warning: '-fcontracts-allow-assume' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-evaluation-semantic=' has no effect without '-fcontracts'
// CHECK-DAG: warning: contract configuration options have no effect without '-fcontracts'
// CHECK-NOT: {{warning|error}}:

// CONFIG: warning: contract configuration options have no effect without '-fcontracts'
// CONFIG-NOT: {{warning|error}}:

int i;
