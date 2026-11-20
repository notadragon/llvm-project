// RUN: %clang_cc1 -std=c++26 -fcontracts -emit-llvm -o - %s | FileCheck %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-configuration-file=%S/p3595-dynamic-cxxname.json -emit-llvm -o - %s | FileCheck %s --check-prefixes=CHECK,DYN

// One contract descriptor table per translation unit, shared by every
// contract check, and one patched violation data block per contract, shared
// by every arm of a dynamic-selection dispatch (CLANG-659: the lookup meant
// to find the first copy skipped internal globals, so each check, and each
// arm, emitted its own).

namespace std::contracts {
  enum class evaluation_semantic : unsigned char {
    ignore = 1, observe, enforce, quick_enforce
  };
}

namespace mylib {
  std::contracts::evaluation_semantic contract_semantic();
}

void f(int x) pre(x > 0) {}
void g(int x) pre(x > 1) {}

// CHECK: @__clang_contract_desc_basic = internal
// CHECK-NOT: @__clang_contract_desc_basic.
// CHECK-NOT: .patched.{{[0-9]+}} =
// DYN: call{{.*}} @_ZN5mylib17contract_semanticEv
