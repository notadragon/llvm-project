// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3097 -fsyntax-only -verify %s

// P3097: Redeclaration of same virtual function must still match contracts.
// Override (different function) is independent — no contract inheritance.

struct Base {
  virtual int f(int x) pre(x > 0) post(r: r > 0);
};

// Redeclaration of same function must match
int Base::f(int x) pre(x > 0) post(r: r > 0) { return x; } // OK

// Different contracts on redeclaration is still an error
struct Bad {
  virtual int g(int x) pre(x > 0); // expected-note {{contract previously specified}}
};
int Bad::g(int x) pre(x > 10) { return x; } // expected-error {{differs in contract specifier sequence}} \
                                             // expected-note {{in contract specified here}}
