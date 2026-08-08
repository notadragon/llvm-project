// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-array-bounds-codegen.json %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// P3100: per-semantic codegen for an out-of-bounds subscript.  assume -> plain
// subscript (no check).  ignore -> the index is replaced by a select
// redirecting an out-of-range value to 0.  observe -> the same select plus a
// branch to the handler.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-array-bounds-codegen.C)

int g[8];
int a_rd(int i) { return g[i]; }                     // assume (no config)
namespace ign_ns { int rd(int i) { return g[i]; } }  // ignore
namespace obs_ns { int rd(int i) { return g[i]; } }  // observe

// CHECK-LABEL: define {{.*}}@_Z4a_rdi
// CHECK-NOT: select
// CHECK-NOT: __cxa_contract_violation

// CHECK-LABEL: define {{.*}}@_ZN6ign_ns2rdEi
// CHECK: select
// CHECK-NOT: __cxa_contract_violation

// CHECK-LABEL: define {{.*}}@_ZN6obs_ns2rdEi
// CHECK: select
// CHECK: br i1
// CHECK: __cxa_contract_violation_implicit_observe
