// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// An automatic object whose destructor is called explicitly is destroyed
// again at the end of its scope ([basic.life]/9), which is undefined, so it
// is not a core constant expression even when the destructor does not touch
// the object.  Clang rejects it; GCC accepts it (stock, gnu_gcc GCC-714,
// deferred), and its watch test is the program below.  Not
// contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-double-destruction-automatic.C

struct X { constexpr ~X() {} };
constexpr bool f() {
  X x; // expected-note {{destroying object 'x' whose lifetime has already ended}}
  x.~X();
  return true; // x is destroyed again at scope exit
}
static_assert(f()); // expected-error {{not an integral constant expression}} \
                    // expected-note {{in call to 'f()'}}
