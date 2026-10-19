// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// The names nominated by a `using contract_control namespace X;' directive
// are found only within assertion-control expressions (P3400), by qualified
// lookup as by unqualified: `::lab' is not ambiguous with the
// contract_control-nominated L::lab, and `N::lab' does not find A::lab.
// (libc++'s <contracts> has such a directive for std::contracts::labels at
// global scope.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-control-qualified-lookup.C

namespace L { int lab = 2; }
using contract_control namespace L;

namespace A { int lab = 1; } // expected-note {{'lab' declared here}}
using namespace A;
int z = ::lab;     // OK, finds A::lab only

namespace N { using contract_control namespace A; }
int w = N::lab;    // expected-error {{no member named 'lab' in namespace 'N'; did you mean simply 'lab'?}}
