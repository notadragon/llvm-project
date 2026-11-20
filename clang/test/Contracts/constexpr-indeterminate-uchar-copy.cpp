// RUN: %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// Copying an indeterminate byte into an unsigned char is well-defined at run
// time ([basic.indet]/2), but in constant evaluation it applies an
// lvalue-to-rvalue conversion to an object with an indeterminate value,
// which [expr.const] excludes from a core constant expression.  So reading a
// padding byte that std::bit_cast copied into an unsigned char array is
// rejected; reading a byte that has a value (the control) is not.  CLANG-690
// asked whether the rejection was a bug; it is not (the user, 2026-10-13,
// DECISIONS.md E79).  GCC rejects it too.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-indeterminate-uchar-copy.C

#include <bit>

struct S { char c; int i; };
struct B { unsigned char b[8]; };

constexpr int f() {
  B a = std::bit_cast<B>(S{1, 2}); // expected-note {{declared here}}
  unsigned char x = a.b[1]; // expected-note {{read of uninitialized object}}
  (void)x;
  return 0;
}
static_assert((f(), true)); // expected-error {{not an integral constant expression}} \
                            // expected-note {{in call to 'f()'}}

constexpr int g() {
  B a = std::bit_cast<B>(S{1, 2});
  unsigned char x = a.b[0]; // the value of c
  return x;
}
static_assert(g() == 1);
