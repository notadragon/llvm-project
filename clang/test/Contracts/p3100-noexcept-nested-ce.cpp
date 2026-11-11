// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3100 -fsyntax-only -verify %s

// P3100: keeping implicit assertions out of the noexcept operator must not
// suppress genuine constant evaluation nested within the operand.  Here f(-1)
// is a template argument (a constant expression) inside a noexcept(...) operand,
// so its precondition x > 0 must still be evaluated and fail.
// (GCC mirror: g++.dg/contracts/cpp26/p3100-noexcept-nested-ce.C)

constexpr int f (int x) pre (x > 0) { return 0; } // expected-error {{contract failed during execution of constexpr function}}

template <int N> bool b = true;

static_assert (noexcept (b<f (-1)>)); // expected-error {{non-type template argument is not a constant expression}} expected-note {{in call to 'f(-1)'}}
