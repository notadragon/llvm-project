// P3290: the __cpp_lib_assert_can_use_contracts feature-test macro is provided
// through <version>, <cassert>, and <assert.h>, and the assert integration
// compiles whether <cassert> or <assert.h> is included, in either order (even
// with __STDC_WANT_ASSERT_USES_CONTRACTS__, which exercises the shared header being processed
// by both wrappers without a redeclaration error).
//
// RUN: %clangxx -std=c++26 -fsyntax-only -fcontracts -fcontracts-p3290 %libcxx_flags %s
// RUN: %clangxx -std=c++26 -fsyntax-only -fcontracts -fcontracts-p3290 %libcxx_flags -DHDR_VERSION %s
// RUN: %clangxx -std=c++26 -fsyntax-only -fcontracts -fcontracts-p3290 %libcxx_flags -DHDR_ASSERT_H %s
// RUN: %clangxx -std=c++26 -fsyntax-only -fcontracts -fcontracts-p3290 %libcxx_flags -DHDR_BOTH_12 -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %s
// RUN: %clangxx -std=c++26 -fsyntax-only -fcontracts -fcontracts-p3290 %libcxx_flags -DHDR_BOTH_21 -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %s

#if defined(HDR_VERSION)
#  include <version>
#elif defined(HDR_ASSERT_H)
#  include <assert.h>
#elif defined(HDR_BOTH_12)
#  include <cassert>
#  include <assert.h>
#elif defined(HDR_BOTH_21)
#  include <assert.h>
#  include <cassert>
#else
#  include <cassert>
#endif

#ifndef __cpp_lib_assert_can_use_contracts
#  error "__cpp_lib_assert_can_use_contracts must be defined"
#endif
static_assert(__cpp_lib_assert_can_use_contracts == 202606L,
              "unexpected __cpp_lib_assert_can_use_contracts value");
