// RUN: %clangxx -std=c++20 %s -fcontracts-p3850 %libcxx_flags -fsyntax-only
// RUN: %clangxx -std=c++23 %s -fcontracts-p3850 %libcxx_flags -fsyntax-only
// RUN: %clangxx -std=c++26 %s -fcontracts-p3850 %libcxx_flags -fsyntax-only

// The feature-test macros must follow the flags, not the dialect.
//
// -fcontracts and the per-paper flags enable contracts at any dialect, and
// below C++26 the library facility works there too, so every macro checked
// here must be defined at all three standards.  The dialect is pinned on each
// RUN line deliberately: until 2026-09-20 libc++'s seven __cpp_lib_* contracts
// entries sat inside `#if _LIBCPP_STD_VER >= 26`, which made them unreachable
// below C++26, and every other feature-test-macro test in this directory runs
// at -std=c++26 only -- the one dialect where the bug is invisible.  A test
// that leaves the dialect to the harness reproduces that blind spot.

#include <contracts>

// -- Language macros, predefined by the flags (InitPreprocessor.cpp). -------

#ifndef __cpp_contracts
#error "__cpp_contracts not defined"
#endif
#ifndef __cpp_contracts_message
#error "__cpp_contracts_message not defined"
#endif
#ifndef __cpp_contracts_postcondition_captures
#error "__cpp_contracts_postcondition_captures not defined"
#endif
#ifndef __cpp_contracts_labels
#error "__cpp_contracts_labels not defined"
#endif
#ifndef __cpp_contracts_requires
#error "__cpp_contracts_requires not defined"
#endif
#ifndef __cpp_contracts_nonthrowing_semantics
#error "__cpp_contracts_nonthrowing_semantics not defined"
#endif
#ifndef __cpp_contracts_report
#error "__cpp_contracts_report not defined"
#endif
#ifndef __clang_contracts_p3290
#error "__clang_contracts_p3290 not defined"
#endif
#ifndef __clang_contracts_p3100
#error "__clang_contracts_p3100 not defined"
#endif

// -- Library macros, defined by <version> off the language macros above. ----

#ifndef __cpp_lib_contracts
#error "__cpp_lib_contracts not defined"
#endif
#ifndef __cpp_lib_contracts_message
#error "__cpp_lib_contracts_message not defined"
#endif
#ifndef __cpp_lib_contracts_report
#error "__cpp_lib_contracts_report not defined"
#endif
#ifndef __cpp_lib_contracts_api
#error "__cpp_lib_contracts_api not defined"
#endif
#ifndef __cpp_lib_assert_can_use_contracts
#error "__cpp_lib_assert_can_use_contracts not defined"
#endif
#ifndef __cpp_lib_contracts_implicit
#error "__cpp_lib_contracts_implicit not defined"
#endif
#ifndef __cpp_lib_contracts_labels
#error "__cpp_lib_contracts_labels not defined"
#endif

static_assert(__cpp_lib_contracts >= 202502L);
// The unadopted papers' macros carry the date of the implemented revision,
// which changes with every revision: only check that they are positive.
static_assert(__cpp_lib_contracts_message > 0);
static_assert(__cpp_lib_contracts_report > 0);
static_assert(__cpp_lib_contracts_api > 0);
static_assert(__cpp_lib_assert_can_use_contracts > 0);
static_assert(__cpp_lib_contracts_implicit > 0);
static_assert(__cpp_lib_contracts_labels > 0);

// The facility itself, not just its macros: a handler signature naming the
// type must compile at every dialect above.
void handle(const std::contracts::contract_violation &);

// REQUIRES: contracts-libcxx
