// RUN: %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// A delete-expression whose operand was not obtained from a non-array
// new-expression is undefined ([expr.delete]/2), here storage from
// std::allocator<int>::allocate, so it is not a core constant expression.
// Clang rejects it; GCC accepts it (stock, gnu_gcc GCC-764, deferred), and
// its watch test is the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-delete-allocator-storage.C

#include <memory>

constexpr bool f() {
  std::allocator<int> a;
  int *p = a.allocate(1); // expected-note {{heap allocation performed here}}
  delete p; // expected-note {{'delete' used to delete pointer to object allocated with 'std::allocator<...>::allocate'}}
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
