// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -fsyntax-only

// Compiler-level vendor macro is defined when -fcontracts-p3290 is set.

#ifndef __clang_contracts_p3290
#error "__clang_contracts_p3290 not defined"
#endif

// Library macro __cpp_lib_contracts_api is defined by <contracts>.
#include <contracts>

#ifndef __cpp_lib_contracts_api
#error "__cpp_lib_contracts_api not defined after including <contracts>"
#endif

// Unadopted: the value is the date of the implemented revision.
static_assert(__cpp_lib_contracts_api > 0,
              "__cpp_lib_contracts_api not positive");

// REQUIRES: contracts-libcxx
