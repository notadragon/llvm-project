// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-invalid-value-codegen.json %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// P3100: invalid bool value load, per-semantic codegen.  assume -> a plain load
// (the value is assumed in range; no check).  ignore -> the storage byte is
// range-checked and an out-of-range value is replaced via a select, with no
// handler.  observe -> the same check branches to the handler, then continues
// with the substituted value.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-invalid-value-codegen.C)

bool a_load(const bool* p) { return *p; }              // assume (no config)
namespace ign_ns { bool load(const bool* p) { return *p; } }  // ignore
namespace obs_ns { bool load(const bool* p) { return *p; } }  // observe

// CHECK-LABEL: define {{.*}}@_Z6a_loadPKb
// CHECK-NOT: select
// CHECK-NOT: __cxa_contract_violation

// CHECK-LABEL: define {{.*}}@_ZN6ign_ns4loadEPKb
// CHECK: select
// CHECK-NOT: __cxa_contract_violation

// CHECK-LABEL: define {{.*}}@_ZN6obs_ns4loadEPKb
// CHECK: select
// CHECK: br i1
// CHECK: __cxa_contract_violation_implicit_observe
