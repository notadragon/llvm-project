// P3595 dynamic-selection constant-evaluation rule (design section 3, half A):
// an entry with both "dynamic" and "semantic" still matches during constant
// evaluation and resolves to "semantic" -- the dynamic selector function is
// never invoked at compile time.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-constexpr.json -fsyntax-only -verify %s

constexpr int g(int x) pre(x > 0) { return x; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int ok = g(1);
constexpr int bad = g(-1); // expected-error {{constexpr variable 'bad' must be initialized by a constant expression}}
  // expected-note@-1 {{in call to 'g(-1)'}}
