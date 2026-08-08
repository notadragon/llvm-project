// [over.call.func]/p3.1: if an unqualified function call/member access appears in
// a *constructor* precondition or a *destructor* postcondition and overload
// resolution selects a non-static member (where the object is not yet, or no
// longer, within its lifetime), the call is ill-formed.
//
// Clang does not yet implement this rule -- it currently accepts an unqualified
// non-static member access in a constructor precondition (GCC diagnoses it:
// g++.dg/contracts/cpp26/over.call.func.p3.1.C). Documented divergence / owed
// base-facility rule; when implemented, drop the expected-failure marker below.
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s
// XFAIL: *

struct S {
  bool f();
  bool k;
  S() pre(k);      // expected-error {{'this' argument required when accessing a member in a constructor precondition}}
  ~S() post(f());  // expected-error {{'this' argument required when accessing a member in a destructor postcondition}}
  S(int) pre(this->k); // qualified form is well-formed
};
