// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A violated precondition of a constexpr destructor, run when a local goes
// out of scope during constant evaluation, is checked, so the static_assert
// is not a constant expression.  The constructor shapes are
// constexpr-ctor-contracts.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-cdtor-contracts.C

struct U {
  int v = 1;
  constexpr ~U() pre(v == 99) {} // expected-error {{contract failed during execution of constexpr function}}
};
constexpr bool fu() { U u; return true; } // expected-note 0+ {{}}
static_assert(fu()); // expected-error {{static assertion expression is not an integral constant expression}} expected-note 0+ {{}}
