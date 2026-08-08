// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-overflow-codegen.json %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// P3100: the signed-overflow check is per-operation and its codegen depends on
// the resolved semantic.  assume -> `add nsw` (the optimizer keeps assuming no
// overflow, so loops built on it stay optimized).  ignore -> plain `add` (no
// nsw), i.e. defined 2's-complement wrapping (-fwrapv-equivalent -- still
// optimizable, but differently from assume).  observe -> a checked
// sadd.with.overflow branching to the handler, then continuing with the wrapped
// result.  (GCC mirror: g++.dg/contracts/cpp26/p3100-overflow-codegen.C.)

int a_add(int a, int b) { return a + b; }              // assume (no config)
namespace ign_ns { int add(int a, int b) { return a + b; } }  // ignore
namespace obs_ns { int add(int a, int b) { return a + b; } }  // observe

// CHECK-LABEL: define {{.*}}@_Z5a_addii
// CHECK: add nsw i32
// CHECK-NOT: sadd.with.overflow

// CHECK-LABEL: define {{.*}}@_ZN6ign_ns3addEii
// CHECK: add i32
// CHECK-NOT: nsw
// CHECK-NOT: sadd.with.overflow

// CHECK-LABEL: define {{.*}}@_ZN6obs_ns3addEii
// CHECK: @llvm.sadd.with.overflow.i32
// CHECK: br i1
// CHECK: __cxa_contract_violation_implicit_observe
