// RUN: %clangxx -std=c++23 -fcontracts-p3850 -fno-contracts-p3400 %s %libcxx_flags -o %t && %t
// RUN: %clang -### -std=c++23 -fcontracts-p3850 -fno-contracts-p3400 %s 2>&1 | FileCheck %s

// DECISIONS.md E175: the per-paper contract flags are additive, in the
// driver as in cc1, so a later -fno- of one per-paper flag leaves contracts
// on, and the contracts runtime linked, while another is on.  -std=c++23 so
// that the flags are the only thing enabling contracts
// (contract-runtime-link-p4451.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-link-p3850-no-p3400.C

// CHECK: "-cc1"
// CHECK-SAME: "-fcontracts"
// CHECK-SAME: "-fcontracts-p3850"
// CHECK: "-lcontracts"

int f(int x) pre(x > 0) { return x; }

int main() { return f(1) - 1; }
