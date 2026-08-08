// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// Captures shadow parameters in the predicate
int g(int x)
  post [x = x + 1] (x > 0) { return x; }

// Multiple captures visible in predicate
int f(int x, int y)
  post [a = x, b = y] (a + b >= 0) { return x + y; }

// Separate declaration + definition: captures rebind correctly
int h(int x) post [x] (x >= 0);
int h(int x) { return x; }
