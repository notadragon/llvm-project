// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A postcondition with a result name, on a function whose return type
// deduction never completes: the predicate, parsed while the result's type is
// still `auto`, is never substituted, and the error is reported without
// anything further going wrong.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/postcondition-undeduced-result.C

bool check(bool b) { return b; }

// Shape 1, the one PR127450 reports: deduction fails because the returned
// expression is ill-formed.
//
// The second diagnostic is error recovery -- with nothing deduced the return
// type falls back to void, and a result name on a void return is separately
// ill-formed.  It is recorded because -verify demands every diagnostic be
// accounted for, not because the cascade is required behaviour.
class S {
  // expected-error@+2 {{use of undeclared identifier 'e'}}
  // expected-error@+1 {{result name 'r' cannot appear in a postcondition for a function with a void return type}}
  auto f() post(r: check(r)) { return e; }
};

// Shape 2: deduction fails with no undeclared name anywhere, which is what
// shows the trigger is "deduction never completed" rather than "the body
// mentioned something undeclared".
bool g();
// expected-error@+2 {{function 'h' with deduced return type cannot be used before it is defined}}
// expected-note@+1 {{'h' declared here}}
auto h() post(r: g()) { return h(); }
