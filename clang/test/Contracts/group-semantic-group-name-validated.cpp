// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-group-evaluation-semantic=a..b:observe -fcontract-group-evaluation-semantic=.a:observe -fsyntax-only %s 2>&1 | FileCheck %s

// The group name in -fcontract-group-evaluation-semantic=group:semantic is
// validated: "a..b" and ".a" are rejected.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3400-group-semantic-group-name.C

// CHECK: error: contract group name 'a..b' cannot contain empty subgroup '..'
// CHECK: error: contract group name '.a' cannot start with '.'

int f(int x) pre(x > 0) { return x; }
