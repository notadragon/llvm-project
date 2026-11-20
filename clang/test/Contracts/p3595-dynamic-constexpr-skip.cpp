// P3595 dynamic-selection constant-evaluation rule (design section 3, half B):
// a dynamic-only entry (no "semantic") is never a match during constant
// evaluation -- there is no compile-time value to fall back to -- so the
// match scan must skip it and fall through to the next entry ("observe"
// here).  If the skip rule were missing, this entry would incorrectly match
// in constant evaluation and its default-initialized Semantic (Enforce)
// would turn the violation below into a hard constexpr error instead of the
// expected warning-and-continue behavior of "observe".
// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fcontract-configuration-file=%S/p3595-dynamic-constexpr-skip.json -fsyntax-only -verify %s

constexpr int g(int x) pre(x > 0) { return x; } // expected-warning {{contract failed during execution of constexpr function}}
constexpr int bad = g(-1);
