// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts %s 2>&1 | FileCheck %s

// -fcontracts on a C input is the C++ option: it draws the usual "argument
// unused" warning, and a link whose inputs are all C does not get
// -lcontracts.  (C contracts are -fcontracts-p4299.)  The related
// clang-cl /std:c++latest case is clang-cl-std-latest.cpp.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-fcontracts-warning.c

// CHECK: warning: {{.*}}-fcontracts
// CHECK-NOT: "-lcontracts"

int f(int x) { return x; }
