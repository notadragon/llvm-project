// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fsyntax-only -verify %s

struct NoCopy {
  NoCopy() = default;
  NoCopy(const NoCopy&) = delete; // expected-note {{'NoCopy' has been explicitly marked deleted here}}
};

void f1(NoCopy nc)
  post [nc] (true); // expected-error {{call to deleted constructor}}
