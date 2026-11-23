// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

int f(int i)
  post [i] (r: r >= 0); // expected-error {{postcondition captures require '-fcontracts-p3098'}}
