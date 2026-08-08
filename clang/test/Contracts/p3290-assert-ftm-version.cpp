// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -fsyntax-only %libcxx_flags

// P3290: __cpp_lib_assert_can_use_contracts is defined via <version>.
// (GCC mirror: p3290-assert-ftm-version.C)

#include <version>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined via <version>"
#endif
