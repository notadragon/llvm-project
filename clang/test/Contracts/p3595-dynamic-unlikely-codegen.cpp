// RUN: %clangxx -std=c++26 %s -fcontracts \
// RUN:   -fcontract-configuration-file=%S/p3595-dynamic-adjust-selector.json \
// RUN:   %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// A dynamically selected check is marked unlikely in each arm that checks
// (notadragon_wg21 DECISIONS-resolved.md E63 (a)): observe, enforce and
// quick_enforce here; the ignore arm emits no check and the
// invalid-selection default is not a check.
// contract-check-unlikely-codegen.cpp covers the static checks.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3595-dynamic-unlikely-codegen.C
// in the gnu_gcc fork.

void f(int x) pre(x > 0) {}

// CHECK-COUNT-3: br i1 {{.*}}, label %{{.*}}, label %{{.*}}, !prof ![[#]]
// CHECK-NOT: !prof
// CHECK: = !{!"branch_weights", {{.*}}i32 {{[0-9]+}}, i32 1}
// CHECK-NOT: branch_weights

// REQUIRES: contracts-libcxx
