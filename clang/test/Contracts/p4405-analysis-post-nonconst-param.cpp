// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=prove -triple x86_64-unknown-linux-gnu -O2 -emit-llvm-only -verify %s
// REQUIRES: x86-registered-target

// GCC-938's case: a postcondition that odr-uses a value parameter that is
// not const is ill-formed ([dcl.contract.func]); once that is diagnosed,
// its D4405 analysis evaluation is not made -- no second error for the
// same assertion -- in a function or in a function template's
// specialization.  Clang never had the cascade: an error stops it before
// code generation, which makes the analysis evaluations.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-post-nonconst-param.C

int f(int x)                    // expected-note {{parameter of type 'int' is declared here}}
  post(r: r > x)                // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}}
{
  return x + 1;
}

template <typename T>
T h(T x)                        // expected-note {{parameter of type 'int' is declared here}}
  post(r: r > x)                // expected-error {{parameter 'x' referenced in contract postcondition must be declared const}}
{
  return x + 1;
}

int call() { return h(1); }     // expected-note {{in instantiation of function template specialization 'h<int>' requested here}}
