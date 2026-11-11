// RUN: %clangxx %s -fsanitize=integer-divide-by-zero -ffile-prefix-map=/src=/mapped %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s
// RUN: %clangxx %s -fsanitize=integer-divide-by-zero -fmacro-prefix-map=/src=/mapped %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s
// RUN: %clangxx %s -fsanitize=integer-divide-by-zero -ffile-prefix-map=/src=/mapped/dir -fsanitize-undefined-strip-path-components=1 %libcxx_flags -o %t
// RUN: %t 2>&1 | FileCheck %s --check-prefix=STRIP

// CLANG-881: -ffile-prefix-map and -fmacro-prefix-map remap the filename in
// a UBSan report as they remap __FILE__, before
// -fsanitize-undefined-strip-path-components= strips it.  The #line
// directive gives the file a fixed name to map.
//
// Mirror: gcc/testsuite/c-c++-common/ubsan/file-prefix-map-1.c

#include <cstdio>

#line 1 "/src/repro.cpp"
int main() {
  volatile int a = 1, b = 0;
  std::fprintf(stderr, "__FILE__ is %s\n", __FILE__);
  (void)(a / b);
  return 0;
}

// CHECK: __FILE__ is /mapped/repro.cpp
// CHECK: /mapped/repro.cpp:{{[0-9]+}}:{{[0-9]+}}: runtime error: division by zero

// STRIP: __FILE__ is /mapped/dir/repro.cpp
// STRIP: {{^}}mapped/dir/repro.cpp:{{[0-9]+}}:{{[0-9]+}}: runtime error: division by zero
