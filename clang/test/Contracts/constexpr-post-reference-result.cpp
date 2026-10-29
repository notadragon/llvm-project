// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A postcondition naming the result of a reference-returning constexpr
// function is evaluated during constant evaluation with the result name
// bound to the returned lvalue; decltype(auto) returning a parenthesized
// name too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-post-reference-result.C

constexpr const int &pick(const int &a) post(r : r > 0) { return a; } // expected-error {{contract failed during execution of constexpr function}}
constexpr int g = 5;
constexpr int v = pick(g); // OK
static_assert(v == 5);
constexpr int z = 0;
constexpr int bad = pick(z); // expected-error {{constexpr variable 'bad' must be initialized by a constant expression}} expected-note {{in call to 'pick(z)'}}

constexpr int gx = 5;
constexpr decltype(auto) pick2() post(r : r > 0) { return (gx); }
constexpr int w = pick2(); // OK
