// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=noexcept_enforce -fsyntax-only %s > %t.out 2>&1 || true
// RUN: FileCheck %s < %t.out

// A P4298 noexcept semantic requested without -fcontracts-p4298 is
// downgraded to its plain counterpart, and the compiler says so.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p4298-semantic-without-flag.C
//
//
//
//
//

// CHECK: {{warning|error}}: {{.*}}-fcontracts-p4298

int f(int x) pre(x > 0) { return x; }
