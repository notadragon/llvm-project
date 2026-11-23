// RUN: %clangxx %s -fsanitize=address -ffile-prefix-map=%S=/mapped %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s
// RUN: %clangxx %s -fsanitize=address -fmacro-prefix-map=%S=/mapped %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s

// CLANG-881: -ffile-prefix-map and -fmacro-prefix-map remap the module name
// ASan records for a global, which its description of the global ("defined
// in") and a global-buffer-overflow report on it print, as they remap
// __FILE__.
//
// Mirror: gcc/testsuite/c-c++-common/asan/global-file-prefix-map-1.c

#include <cstdio>
#include <sanitizer/asan_interface.h>

char XXX[10];

int main() {
  std::fprintf(stderr, "__FILE__ is %s\n", __FILE__);
  __asan_describe_address(XXX + 10);
  return 0;
}

// CHECK: __FILE__ is /mapped/sanitizer-prefix-map-asan.cpp
// CHECK: global variable 'XXX' defined in '/mapped/sanitizer-prefix-map-asan.cpp'
