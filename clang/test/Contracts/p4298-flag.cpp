// D4298: noexcept_enforce/noexcept_observe are only valid evaluation
// semantics when -fcontracts-p4298 is enabled.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce %s -emit-llvm -o - | FileCheck %s
// CHECK: define{{.*}} @_Z1fi
int f(int x) pre(x > 0) { return x; }
