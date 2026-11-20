// RUN: %clangxx -std=c++23 -fcontracts-p3400 -fno-contracts-p3850 %s %libcxx_flags -o %t && %t
// RUN: %clang -### -std=c++23 -fcontracts-p3400 -fno-contracts-p3850 %s 2>&1 | FileCheck %s

// DECISIONS.md E175: the per-paper contract flags are additive, in the
// driver as in cc1, so a later -fno- of one per-paper flag leaves contracts
// on, and the contracts runtime linked, while another is on.  -std=c++23 so
// that the flags are the only thing enabling contracts
// (contract-runtime-link-p4451.cpp).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-link-p3400-no-p3850.C

// CHECK: "-cc1"
// CHECK-SAME: "-fcontracts"
// CHECK-SAME: "-fcontracts-p3400"
// CHECK: "-lcontracts"

int f(int x) pre(x > 0) { return x; }

int main() { return f(1) - 1; }
