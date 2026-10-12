// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -DNDEBUG \
// RUN:   -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %libcxx_flags -o %t
// RUN: %t

// P3290: assert is disabled by NDEBUG even when __STDC_WANT_ASSERT_USES_CONTRACTS__ is set.
// (GCC mirror: p3290-assert-ndebug.C)

#include <cassert>
#include <cstdio>

int main() {
  assert(1 == 2);  // compiled out by NDEBUG
  std::printf("PASS\n");
}
