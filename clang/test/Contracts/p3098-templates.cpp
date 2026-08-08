// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s
// expected-no-diagnostics

template<typename T>
T identity(T x) post [old = x] (old >= 0) { return x; }

template int identity<int>(int);

template<typename T>
T add(T a, T b) post [a, b] (a + b >= 0) { return a + b; }

template int add<int>(int, int);
