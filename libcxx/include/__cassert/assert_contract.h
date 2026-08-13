// -*- C++ -*-
//===----------------------------------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

// P3290 assert integration, shared by <cassert> and <assert.h>.
//
// This header intentionally has NO include guard: like <cassert>, the `assert`
// macro must be (re)defined according to the current NDEBUG each time it is
// textually included.  Includers must include the C <assert.h> BEFORE this
// header so that the contract definition of `assert` wins.

#include <__config>

#if !defined(_LIBCPP_HAS_NO_PRAGMA_SYSTEM_HEADER)
#  pragma GCC system_header
#endif

// `__clang_contracts_p3290` is predefined by the compiler when
// -fcontracts-p3290 (or -fcontracts-p3850) is active; it tells the library to
// provide the P3290 assert integration and its feature-test macro.
#if defined(__cplusplus) && __cplusplus > 202302L && defined(__clang_contracts_p3290)

// Capability feature-test macro (P3290): defined whenever the platform supports
// the assert integration, independent of whether __STDC_WANT_ASSERT_USES_CONTRACTS__ is
// defined at the point of inclusion.
#  if !defined(__cpp_lib_assert_can_use_contracts)
#    define __cpp_lib_assert_can_use_contracts 202606L
#  endif

#  if !defined(NDEBUG) && defined(__STDC_WANT_ASSERT_USES_CONTRACTS__)
#    include <source_location>
// Shared assert-integration entry point (P3290): reports assertion_kind=cassert
// and detection_mode=predicate_false to the contract-violation handler, and
// terminates via std::abort() on any completion of the handler (a normal return
// or an escaping exception).  libstdc++ provides the same symbol.  The
// source_location is passed from the macro (rather than via a default argument)
// so this declaration is safe to re-process across repeated inclusions.
extern "C++" [[noreturn]] void __cxa_handle_cassert_violation(const char*, ::std::source_location) noexcept;
#    undef assert
#    define assert(...)                                                                                                \
      ((__VA_ARGS__) ? (void)0 : __cxa_handle_cassert_violation(#__VA_ARGS__, ::std::source_location::current()))
#  endif // !NDEBUG && __STDC_WANT_ASSERT_USES_CONTRACTS__

#endif // C++26 && __clang_contracts_p3290
