// RUN: %clangxx_default_config -std=c++26 -stdlib=libstdc++ -fsyntax-only \
// RUN:   -Xclang -verify %s
// RUN: %clangxx_default_config -std=c++26 -stdlib=libstdc++ \
// RUN:   -fcontract-evaluation-semantic=ignore -S -emit-llvm -o - %s \
// RUN:   | FileCheck %s
// REQUIRES: contracts-libstdcxx
// expected-no-diagnostics

// P4451 with libstdc++: under -std=c++26, which implies P4451, libstdc++'s
// <contracts> declares std::contracts::controls::enforced for this compiler
// too, so the label works with either standard library.  A terminating
// contract assertion is evaluated with enforce although ignore is
// configured.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-ftm.C (the library
// declaration under GCC)

#include <contracts>  // `std::contracts::controls::enforced`

#if !(__clang_contracts_p4451 > 0)
#error "__clang_contracts_p4451"
#endif

namespace scl = std::contracts::controls;

// CHECK-LABEL: define {{.*}}@_Z1fi(
// CHECK: call void @__cxa_contract_violation_pre_enforce
void f(int x) pre<std::contracts::controls::enforced>(x > 0) {}

// CHECK-LABEL: define {{.*}}@_Z1gi(
// CHECK: call void @__cxa_contract_violation_assert_enforce
void g(int x) { contract_assert<scl::enforced>(x > 0); }
