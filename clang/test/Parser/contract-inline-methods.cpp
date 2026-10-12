// RUN: %clang_cc1 -std=c++26 -fsyntax-only -verify=expected %s -fcontracts

// A contract on an inline member function is parsed once the class is
// complete, and still diagnoses a name that does not exist; a declaration's
// contract may be repeated on its out-of-line definition.

struct Foo {
  int f(int x) pre(xy != 0) { // expected-error {{use of undeclared identifier 'xy'}}
    return x;
  }
  int g(int x) pre(x != 0);
};

int Foo::g(int x) pre(x != 0) {
  return x;
}
