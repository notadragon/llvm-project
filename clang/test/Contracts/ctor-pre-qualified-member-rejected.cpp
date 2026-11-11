// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// In a constructor precondition or destructor postcondition, a potentially
// evaluated qualified-id naming a non-static member (C::b) is transformed to
// a class member access and is ill-formed ([expr.prim.id.general]), like the
// unqualified b.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ctor-pre-qualified-member.C

struct C {
  bool b;
  C(char) pre(C::b);      // expected-error {{cannot access non-static member 'b' in the precondition of a constructor}}
  C(short) pre(&C::b != nullptr); // expected-warning {{comparison of address of 'C::b' not equal to a null pointer is always true}}
};

struct E {
  bool b;
  ~E() post(E::b);        // expected-error {{cannot access non-static member 'b' in the postcondition of a destructor}}
};
