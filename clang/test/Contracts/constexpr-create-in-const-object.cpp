// RUN: %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// Creating an object in the storage of a const complete object is undefined
// ([basic.life]/10), so it is not a core constant expression.  Clang rejects
// it; GCC accepts it (stock, gnu_gcc GCC-745, PR119304, deferred), and its
// watch test is the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-create-in-const-object.C

#include <memory>

struct B { int x; };
constexpr int f() {
  const B b{1};
  std::construct_at(const_cast<B *>(&b), 2); // expected-note {{in call to}}
  return b.x;
}
static_assert(f() == 2); // expected-error {{not an integral constant expression}} \
                         // expected-note {{in call to 'f()'}}
// expected-note@*:* {{modification of object of const-qualified type 'const B'}}
