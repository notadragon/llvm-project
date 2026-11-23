// RUN: %clang -std=c++23 -fcontracts -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fno-contract-bypass-rethrowing-local-handler -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E177: the contracts option rule.  With contracts on, the
// contracts options that would mean nothing with contracts off do not warn.
// gnu_gcc's -fcontracts-client-check=, -fcontracts-definition-check=,
// -fcontract-checks-outlined, -fcontract-disable-optimized-checks and
// -fcontract-comment-from-expression have no Clang counterpart.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-contracts-on.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

int f(int x) pre(x > 0) { return x; }
