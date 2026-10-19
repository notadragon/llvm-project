// RUN: not %clang_cc1 -std=c++26 -fcontracts -fcontract-group-evaluation-semantic=foo:bogus -fsyntax-only %s 2>&1 | FileCheck %s

// For -fcontract-group-evaluation-semantic=foo:bogus the diagnostic names
// "bogus" as an invalid semantic, not as an invalid group name.  The
// empty-argument shape is group-semantic-empty.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/group-semantic-invalid-semantic.C

// CHECK: error: {{.*}}semantic{{.*}}bogus
// CHECK-NOT: group name "bogus"

int f(int x) pre(x > 0) { return x; }
