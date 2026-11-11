// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A structured binding whose variable is declared outside a contract
// predicate names a const lvalue in it ([expr.prim.id.unqual]), whatever its
// form -- tuple-like, array or aggregate, by value or by reference
// (DECISIONS.md M4).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constify-binding-ref-nttp.C

struct P { int x, y; };

void b1() {
  auto [a, b] = P{};
  contract_assert(++a > 0); // expected-error {{cannot assign to}}
}

void b2() {
  int arr[1] = {0};
  auto &[a] = arr;
  contract_assert(++a > 0); // expected-error {{cannot assign to}}
}

void b3(P p) {
  auto &[a, b] = p;
  contract_assert((b = 1) > 0); // expected-error {{cannot assign to}}
}

// Control, already diagnosed.
void c1(int x) {
  int &r = x;
  contract_assert(++r > 0); // expected-error {{cannot assign to}}
}
