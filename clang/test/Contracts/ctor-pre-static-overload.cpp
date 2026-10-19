// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// An unqualified call in a constructor precondition or destructor
// postcondition is ill-formed only if overload resolution selects a
// non-static member function ([over.call.func]/3): selecting the static
// member of a mixed set is fine, selecting a non-static one is not.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ctor-pre-static-overload.C

struct S {
  bool mixed(int) const;
  static bool mixed(double);
  static bool sf(int);
  S(short) pre(sf(1));
  S(char) pre(mixed(1.0));
  ~S() post(mixed(1.0));
  S(int) pre(mixed(1)); // expected-error {{call to non-static member function without an object argument}}
};
