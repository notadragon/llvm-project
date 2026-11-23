// RUN: %clang_cc1 -fcontracts-p4299 -fsyntax-only -verify %s

// A C contract predicate is a conditional-expression, as in C++, so a comma
// or assignment expression needs parentheses -- on a declaration, a
// definition and a _ContractAssert alike.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-predicate-conditional-expression.c
// (definitions and _ContractAssert) and
// gcc/testsuite/gcc.dg/contracts/c-decl-contract-parsed.c
// (declarations) in the gnu_gcc fork.

int g(int x) _Pre(x, 1);        // expected-error {{expected ')'}} expected-note {{to match this '('}}
int h(int x) _Post(r: r, 1);    // expected-error {{expected ')'}} expected-note {{to match this '('}}
int k(int x) _Pre(x = 1);       // expected-error {{expected ')'}} expected-note {{to match this '('}}

int g2(int x) _Pre(x, 1) { return x; }     // expected-error {{expected ')'}} expected-note {{to match this '('}}
int h2(int x) _Post(r: r, 1) { return x; } // expected-error {{expected ')'}} expected-note {{to match this '('}}
int k2(int x) _Pre(x = 1) { return x; }    // expected-error {{expected ')'}} expected-note {{to match this '('}}

int f(int x) {
  _ContractAssert(x, 1);        // expected-error {{expected ')'}} expected-note {{to match this '('}}
  _ContractAssert(x = 1);       // expected-error {{expected ')'}} expected-note {{to match this '('}}
  _ContractAssert((x, 1));      // expected-warning {{left operand of comma operator has no effect}}
  return x;
}
