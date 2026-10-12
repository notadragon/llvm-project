// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// Within a contract predicate `this` is a pointer to const
// ([expr.prim.this]): explicitly, through an implicit member access, and
// where a lambda in the predicate captures it -- so the lambda cannot modify
// the object through it; it can read it.  A lambda's copy of *this is its
// own member, which a mutable lambda may modify ([expr.prim.id.unqual]).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-this-modifies-object.C

struct S {
  int m;
  void f() pre([this] { return ++m > 0; }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  void g() {
    contract_assert([this] { return ++m > 0; }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
    contract_assert([&] { return ++m > 0; }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
    contract_assert([this] { return m > 0; }()); // ok, a read
  }
  void h() pre([=, this] { return ++m > 0; }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  static void sf(S *) {} // expected-note 2 {{passing argument to parameter here}}
  void i() pre([p = this] { return ++p->m > 0; }()); // expected-error {{cannot assign to variable 'p' with const-qualified type 'const S *'}} expected-note {{variable 'p' declared const here}}
  void j() pre([&] { return [&] { return ++m > 0; }(); }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  void k() pre([this] { return [=, this]() mutable { return ++m > 0; }(); }()); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  void n() pre((sf(this), true)); // expected-error {{cannot initialize a parameter of type 'S *' with an rvalue of type 'const S *'}}
  void o() pre((++this->m, true)); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  // In a lambda's body, a contract_assert's `this` is the enclosing one.
  void q() { [this] { contract_assert(++m > 0); }(); } // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  void r() { [this] { contract_assert((sf(this), true)); }(); } // expected-error {{cannot initialize a parameter of type 'S *' with an rvalue of type 'const S *'}}
  void s() { [this] { contract_assert([&] { return ++m > 0; }()); }(); } // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
  void t() { contract_assert([*this]() mutable { return ++m > 0; }()); } // ok, the copy
};
