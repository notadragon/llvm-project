// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// Without -fcontracts-p3097, a contract on a virtual function -- a base's, or
// an override's -- is an error.

struct Base {
  virtual void f() pre(true); // expected-error {{contracts on virtual functions require '-fcontracts-p3097'}}
};

struct Child : Base {
  void f() override pre(true); // expected-error {{contracts on virtual functions require '-fcontracts-p3097'}}
};
