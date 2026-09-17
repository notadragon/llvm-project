// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// P4283: Rejected without flag.

template<typename T>
void f(T x)
  pre requires(true) (x > 0); // expected-error {{requires clauses on contract assertions require '-fcontracts-p4283'}}
// The parser skips to ( and parses the predicate, so no further errors expected.
