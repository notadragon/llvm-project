// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s \
// RUN:   -fsanitize=integer-divide-by-zero,address \
// RUN:   -fmacro-prefix-map=/src=/mapped -fmacro-prefix-map=%S=/dir \
// RUN:   | FileCheck %s --implicit-check-not='c"/src/' \
// RUN:     --implicit-check-not='c"%S/'
// RUN: %clang_cc1 -triple x86_64-unknown-linux-gnu -emit-llvm -o - %s \
// RUN:   -fsanitize=integer-divide-by-zero,address \
// RUN:   | FileCheck %s --check-prefix=UNMAPPED

// CLANG-881: the filename of a UBSan source location and the module name
// ASan records are remapped by the macro prefix map (which the driver's
// -ffile-prefix-map implies) as __FILE__ is; without a map they are as
// before.  The module name is the main file's name, which #line does not
// change, so it takes the second map.
//
// Mirror: gcc/testsuite/c-c++-common/ubsan/file-prefix-map-1.c and
// gcc/testsuite/c-c++-common/asan/global-file-prefix-map-1.c

#line 1 "/src/repro.cpp"
int g[10];
int div(int a, int b) { return a / b; }

// CHECK-DAG: c"/mapped/repro.cpp\00"
// CHECK-DAG: c"/dir/sanitizer-prefix-map-ir.cpp\00"

// UNMAPPED-DAG: c"/src/repro.cpp\00"
// UNMAPPED-DAG: c"{{.*}}/sanitizer-prefix-map-ir.cpp\00"
