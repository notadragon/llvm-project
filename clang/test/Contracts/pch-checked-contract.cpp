// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -x c++-header %s -emit-pch -o %t.pch
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -x c++ -include-pch %t.pch -DUSE_PCH -emit-llvm -o - %s | FileCheck %s

// Precompiling a header that defines a function with a checked contract; a
// use through the PCH still calls the checked function.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/pch-checked-contract.C

#ifndef USE_PCH
inline int nf(int x) pre(x > 0) { return x; }
#else
int use() { return nf(1); }
#endif

// CHECK: define {{.*}} @_Z2nfi(
// CHECK: contract.violation
