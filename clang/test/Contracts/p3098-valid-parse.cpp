// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

// --- Combined declaration + definition ---

int f1(int i)
  post [i] (i >= 0) { return i; }

int f2(int i, int j)
  post [i, j] (i >= 0 && j >= 0) { return i > j ? i : j; }

int f3(int i)
  post [old_i = i] (old_i >= 0) { return i; }

int f4()
  post [x = 42] (x > 0) { return 100; }

int f5(int i)
  post [i] (i >= 0)
  pre (i >= 0) { return i; }

int f6(int i)
  post [a = i] (a >= 0)
  post [b = i + 1] (b > 0) { return i + 2; }

int f7(int i)
  post [i] (i >= 0)
  post (true) { return i + 1; }

int f8(int i)
  pre (i >= 0)
  post [i] (i >= 0) { return i; }

// --- Separate declaration + definition ---

int g1(int i) post [i] (i >= 0);
int g1(int i) { return i; }

int g2(int i) post [old = i] (old >= 0);
int g2(int i) { return i + 1; }

int g3(int a, int b) post [a, b] (a + b >= 0);
int g3(int a, int b) { return a + b; }
