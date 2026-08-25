// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 %libcxx_flags -fsyntax-only

// Library macro __cpp_lib_contracts_implicit is defined by <contracts>
// when -fcontracts-p3100 is set.

#include <contracts>

#ifndef __cpp_lib_contracts_implicit
#error "__cpp_lib_contracts_implicit not defined after including <contracts>"
#endif

static_assert(__cpp_lib_contracts_implicit >= 202608L,
              "__cpp_lib_contracts_implicit value too low");
