// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// The operand of an assertion-control-expression is a constant-expression
// ([expr.contract.control], P3400): non-constant operands are rejected.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-control-nonconstant.C

struct L { using assertion_control_object = L; int v; };

int g(int x) { // expected-note {{declared here}}
  L l{x}; // expected-note {{declared here}}
  auto a = contract_control(l);      // expected-error {{constant expression}} expected-note {{read of non-constexpr variable 'l'}} expected-note {{in call to 'L(l)'}}
  auto b = contract_control(x + 1);  // expected-error {{constant expression}} expected-note {{function parameter 'x' with unknown value}}
  return a.v + b;
}
