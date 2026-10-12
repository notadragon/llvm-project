// RUN: %clangxx -std=c++17 %libcxx_flags -fsyntax-only -Xclang -verify %s
// RUN: %clangxx -std=c++17 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// libc++'s <contracts> is empty below C++20 (its interface names
// std::source_location), as libstdc++'s is, and including it there is not an
// error, with or without -fcontracts.  The library feature-test macros are not
// defined there either.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contracts-header-below-cxx20.C

#include <contracts>

#ifdef __cpp_lib_contracts
#error "__cpp_lib_contracts defined below C++20"
#endif

#ifdef __cpp_contracts
int f(int x) pre(x > 0) { return x; }
#endif

// REQUIRES: contracts-libcxx
