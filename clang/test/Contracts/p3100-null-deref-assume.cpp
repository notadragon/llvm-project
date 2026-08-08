// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type %libcxx_flags -S -emit-llvm -o - | FileCheck %s

// P3100: under the default "assume" semantic (no configuration file), a null
// dereference is byte-identical to no P3100 -- no null test, no trap, no
// handler call is emitted.  `*p` is an lvalue with no defined substitute, so
// "assume" leaves the raw load untouched.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-null-deref-assume.C)

#include <contracts>

int load_it(int *p) { return *p; }
void store_it(int *p, int v) { *p = v; }

// CHECK-LABEL: define {{.*}}load_it
// CHECK-NOT: llvm.trap
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: nulldref
// CHECK-LABEL: define {{.*}}store_it
// CHECK-NOT: llvm.trap
// CHECK-NOT: __cxa_contract_violation
// CHECK-NOT: nulldref
