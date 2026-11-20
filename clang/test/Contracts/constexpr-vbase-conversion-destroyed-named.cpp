// RUN: %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// Converting a named object outside its lifetime to a virtual base is
// undefined ([basic.life]/8), so it is not a core constant expression.  This
// branch rejects it, by CLANG-1's fix (stock Clang accepts it: CLANG-1 in
// bug-reports/, the explicit-destructor form of its deleted-object cases).
// GCC accepts it (stock, gnu_gcc GCC-750, deferred), and its watch test is
// the program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-vbase-conversion-destroyed-named.C

#include <memory>

struct V { int v = 1; };
struct D : virtual V { constexpr ~D() {} };
constexpr bool f() {
  D d; // expected-note {{declared here}}
  d.~D();
  V &r = d; // expected-note {{conversion to a virtual base of object outside its lifetime}}
  (void)r;
  std::construct_at(&d);
  return true;
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
