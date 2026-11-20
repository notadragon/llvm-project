// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce \
// RUN:   %t/mod.cppm -emit-module-interface -o %t/mod.pcm
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce \
// RUN:   -fprebuilt-module-path=%t %t/use.cpp -emit-llvm -o - | FileCheck %s

// An exported inline function with a checked contract: the module interface
// is written, and the importer's copy of the function is still checked.
//
// GCC: gcc/testsuite/g++.dg/modules/contract-inline-checked_a.C and _b.C

//--- mod.cppm
export module mod;
export inline int ni(int x) pre(x > 0) { return x; }

//--- use.cpp
import mod;
int use() { return ni(1); }

// CHECK: define {{.*}} @_ZW3mod2nii(
// CHECK: contract.violation
