// RUN: %clangxx -std=c++23 -fcontracts-p4451 %s %libcxx_flags -o %t && %t
// RUN: %clang -### -std=c++23 -fcontracts-p4451 %s 2>&1 | FileCheck %s

// A per-paper contract flag enables contracts and links the contracts
// runtime on its own: -fcontracts-p4451 is among the driver's contracts
// sub-flags (tools::wantsCxxContracts) and is forwarded.  -std=c++23 so
// that the flag is the only thing enabling contracts.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-runtime-link-p4451.C

// CHECK: "-cc1"
// CHECK-SAME: "-fcontracts"
// CHECK-SAME: "-fcontracts-p4451"
// CHECK: "-lcontracts"

int f(int x) pre(x > 0) { return x; }

int main() { return f(1) - 1; }
