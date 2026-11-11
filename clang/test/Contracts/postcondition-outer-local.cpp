// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A postcondition of a local-class member function that names an automatic
// variable, or a lambda's by-copy capture, of the enclosing function is
// ill-formed.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-outer-local.C

int f() {
  int local = 5; // expected-note {{'local' declared here}}
  struct L { static int g() post(r : r == local) { return 0; } }; // expected-error {{reference to local variable 'local' declared in enclosing function 'f'}}
  return L::g();
}

int h() {
  int x = 5; // expected-note {{'x' declared here}}
  auto l = [x] {
    struct M { static int g() post(r : r == x) { return 0; } }; // expected-error {{reference to local variable 'x' declared in enclosing function 'h'}}
    return M::g();
  };
  return l();
}
