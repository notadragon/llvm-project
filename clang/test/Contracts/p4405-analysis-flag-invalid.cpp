// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=enforce -fsyntax-only %s 2>&1 | FileCheck %s
// RUN: not %clang -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=observe -fsyntax-only %s 2>&1 | FileCheck %s --check-prefix=DRIVER

// D4405: the analysis evaluation's semantic is one of ignore, diagnose and
// prove; -fcontract-analysis-semantic= rejects any other.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-flag-invalid.C
// (Clang's wording: the value is not one the option takes.)

// CHECK: error: invalid value 'enforce' in '-fcontract-analysis-semantic=enforce'
// DRIVER: error: invalid value 'observe' in '-fcontract-analysis-semantic=observe'

int f(int x) pre(x > 0) { return x; }
