// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

#ifndef __cpp_contracts_labels
#error "__cpp_contracts_labels not defined"
#endif

static_assert(__cpp_contracts_labels >= 202606L,
              "__cpp_contracts_labels value too low");
