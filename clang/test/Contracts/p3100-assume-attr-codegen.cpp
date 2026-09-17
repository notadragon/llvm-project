// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-assume-attr-codegen.json %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// P3100: codegen for a configurable [[assume]] across semantics.  The optimizer
// hint (@llvm.assume) is emitted only where the predicate is guaranteed to hold:
// the default assume, and after a passing enforcing check.  ignore emits
// nothing; observe emits the check (handler) but no hint.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-assume-attr-codegen.C)

int def_f(int x) { [[assume(x > 0)]]; return x; }                    // assume: hint only
namespace ign_ns { int f(int x) { [[assume(x > 0)]]; return x; } }   // ignore: nothing
namespace obs_ns { int f(int x) { [[assume(x > 0)]]; return x; } }   // observe: check, no hint
namespace enf_ns { int f(int x) { [[assume(x > 0)]]; return x; } }   // enforce: check + hint

// CHECK-LABEL: define {{.*}}def_f
// CHECK: call void @llvm.assume
// CHECK-LABEL: define {{.*}}ign_ns
// CHECK-NOT: @llvm.assume
// CHECK-NOT: __cxa_contract_violation
// CHECK-LABEL: define {{.*}}obs_ns
// CHECK: __cxa_contract_violation_implicit_observe
// CHECK-NOT: @llvm.assume
// CHECK-LABEL: define {{.*}}enf_ns
// CHECK: __cxa_contract_violation_implicit_enforce
// CHECK: call void @llvm.assume
