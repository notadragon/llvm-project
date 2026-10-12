// RUN: not %clang -std=c++26 -fcontracts -fcontract-group-evaluation-semantic= -fsyntax-only %s 2>&1 | FileCheck %s

// An empty -fcontract-group-evaluation-semantic= is rejected.  The
// invalid-semantic shape is group-semantic-invalid-semantic.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/group-semantic-empty.C

// CHECK: error: {{.*}}-fcontract-group-evaluation-semantic=

int f(int x) pre(x > 0) { return x; }
