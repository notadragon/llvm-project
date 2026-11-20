// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// expected-no-diagnostics

// In a constructor precondition or destructor postcondition an unevaluated
// use of a non-static member is valid, including the example in
// [expr.prim.id.general] (`C(int) pre(sizeof(b) > 0);  // OK`): the
// restriction applies only when the id-expression is potentially evaluated.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ctor-pre-unevaluated-member.C

struct C {
  bool b;
  int f() const;
  C(int) pre(sizeof(b) > 0);          // OK, the standard's example
  C(float) pre(sizeof(f()) > 0);      // OK, unevaluated
  C(double) pre(noexcept(f()));       // OK, unevaluated
  C(unsigned) pre(requires { b; });   // OK, unevaluated
};

struct D {
  bool b;
  ~D() post(sizeof(b) > 0);           // OK, unevaluated
};
