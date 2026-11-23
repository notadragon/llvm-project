// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -flto=full -emit-llvm \
// RUN:   -o - %s -fsanitize=address -fmacro-prefix-map=%S=/dir \
// RUN:   | FileCheck %s --implicit-check-not='c"%S/'
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -flto=full -O2 -emit-llvm \
// RUN:   -o - %s -fsanitize=address -fmacro-prefix-map=%S=/dir \
// RUN:   | FileCheck %s --implicit-check-not='c"%S/'
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -flto=thin -emit-llvm \
// RUN:   -o - %s -fsanitize=address -fmacro-prefix-map=%S=/dir \
// RUN:   | FileCheck %s --implicit-check-not='c"%S/'
// REQUIRES: x86-registered-target

// CLANG-881 under LTO: the module name ASan records for a global honours
// -fmacro-prefix-map with -flto too.  Clang instruments, and records the
// remapped name, in the compile step, so the bitcode an LTO link reads
// already carries it.  (This build has no LTO-capable linker, so the test
// reads the pre-link IR rather than running a linked program.)
//
// Mirror: gcc/testsuite/g++.dg/asan/open-bug-global-macro-prefix-map-lto.C

char XXX[10];

int main() { return XXX[0]; }

// CHECK: c"/dir/sanitizer-prefix-map-asan-lto.cpp\00"
