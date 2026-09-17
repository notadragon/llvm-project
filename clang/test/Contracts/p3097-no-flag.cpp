// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

struct Base {
  virtual void f() pre(true); // expected-error {{contracts on virtual functions require '-fcontracts-p3097'}}
};

struct Child : Base {
  void f() override pre(true); // expected-error {{contracts on virtual functions require '-fcontracts-p3097'}}
};
