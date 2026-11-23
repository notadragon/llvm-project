// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A postcondition result name that conflicts with a declaration in the nearest
// enclosing lambda scope -- an init-capture -- makes the program ill-formed
// ([basic.scope.contract]); a simple capture declares nothing, and a local
// class's member function inside the lambda is not the lambda.  The parameter
// case is result-name-shadows-parameter.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-shadows-parameter.C

auto l = [x = 1](int y) post(x : x > 0) { return y; }; // expected-error {{declaration of result name 'x' shadows lambda capture}} expected-note {{previous declaration is here}}
void k() {
  int x = 1;
  auto m = [x](int y) post(x : x > 0) { return y + x; }; // OK
  auto n = [z = 1] {
    struct L { int f() post(z : z > 0) { return 1; } }; // OK: L::f is not the lambda
    return L{}.f();
  };
}
