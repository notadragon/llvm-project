// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -o %t
// RUN: %t

// P3290: the assert macro is unchanged without __STDC_WANT_ASSERT_USES_CONTRACTS__.
// (GCC mirror: p3290-assert-default.C)

#include <cassert>
#include <cstdio>

int main() {
  assert(1 == 1);
  std::printf("PASS\n");
}
