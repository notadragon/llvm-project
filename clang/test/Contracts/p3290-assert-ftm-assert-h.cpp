// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -fsyntax-only %libcxx_flags

// P3290: __cpp_lib_assert_can_use_contracts is defined via <assert.h>.
// (GCC mirror: p3290-assert-ftm-assert-h.C)

#include <assert.h>

#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined via <assert.h>"
#endif
