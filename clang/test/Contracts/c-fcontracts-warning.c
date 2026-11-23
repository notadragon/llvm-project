// RUN: %clang -### --target=x86_64-unknown-linux-gnu -std=c23 -fcontracts %s 2>&1 | FileCheck %s

// -fcontracts on a C input is the C++ option: it warns that it is valid for
// C++ but not for C, as GCC does (notadragon_wg21 DECISIONS.md E183 (1)), is
// not passed on, and a link whose inputs are all C does not get -lcontracts.
// (C contracts are -fcontracts-p4299.)  The related clang-cl
// /std:c++latest case is clang-cl-std-latest.cpp.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-fcontracts-warning.c

// CHECK: warning: command-line option '-fcontracts' is valid for C++ but not for C
// CHECK-NOT: argument unused
// CHECK: "-cc1"
// CHECK-NOT: "-fcontracts"
// CHECK-NOT: "-lcontracts"

int f(int x) { return x; }
