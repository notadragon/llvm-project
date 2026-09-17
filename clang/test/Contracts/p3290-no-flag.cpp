// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -fsyntax-only

// Without -fcontracts-p3290, neither the compiler nor library macro is defined.

#ifdef __clang_contracts_p3290
#error "__clang_contracts_p3290 should not be defined without -fcontracts-p3290"
#endif

#include <contracts>

#ifdef __cpp_lib_contracts_api
#error "__cpp_lib_contracts_api should not be defined without -fcontracts-p3290"
#endif
