// RUN: %clang -std=c++23 -fcontracts-allow-assume -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=ignore -fcontract-bypass-rethrowing-local-handler -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck %s

// DECISIONS.md E175-E178: the contracts option rule.  The contracts options
// that mean nothing with contracts off warn that they have no effect: here
// contracts are off because the dialect does not turn them on and no option
// does.  A joined option is named without its argument, and none is also
// reported as unused.  The negative forms do not warn
// (contract-options-rule-no-effect-negative.cpp).
// gnu_gcc's -fcontracts-client-check=, -fcontracts-definition-check=,
// -fcontract-checks-outlined, -fcontract-disable-optimized-checks and
// -fcontract-comment-from-expression have no Clang counterpart.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect.C

// CHECK-DAG: warning: '-fcontracts-allow-assume' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-evaluation-semantic=' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-analysis-semantic=' has no effect without '-fcontracts'
// CHECK-DAG: warning: '-fcontract-bypass-rethrowing-local-handler' has no effect without '-fcontracts'
// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifdef __cpp_contracts
#error "contracts are off"
#endif

int i;
