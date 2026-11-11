// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify %s

// A dynamic_cast whose operand points past the end of an array points to no
// object, which is undefined ([expr.dynamic.cast]/7), so it is not a core
// constant expression.  Clang rejects it; GCC accepts it and yields a null
// pointer (stock, gnu_gcc GCC-757, deferred), and its watch test is the
// program below.  Not contracts-specific.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/open-bug-constexpr-dynamic-cast-past-end-downcast.C

struct B { virtual constexpr ~B() {} };
struct D : B {};
constexpr int t(int x) {
  B bs[1];
  B *bp = bs + x;
  D *dp = dynamic_cast<D *>(bp); // expected-note {{dynamic_cast of dereferenced one-past-the-end pointer}}
  return dp == nullptr ? -1 : 0;
}
static_assert(t(1) == -1); // expected-error {{not an integral constant expression}} \
                           // expected-note {{in call to 't(1)'}}
