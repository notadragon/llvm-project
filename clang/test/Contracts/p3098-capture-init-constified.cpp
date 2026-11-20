// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3098 -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A postcondition capture's initializer is in the contract assertion, so an
// entity declared outside the assertion is constified there as in the
// predicate (P3098 keeps that constification and exempts only the captures
// themselves, notadragon_wg21 D3098R3: `post [j = ++i] (j)` is an error).
// So `this' there is a pointer to const, and the capture initialized from it
// is one too (CLANG-641: Clang deduced `S *' and accepted the modification;
// it constified nothing in a capture's initializer).  The capture itself is
// deduced as `auto' would be, without the initializer's const, and may be
// modified.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-this-modifies-object.C
// in the gnu_gcc fork (its `u').

int i = 0;
void g() post [j = ++i] (j > 0); // expected-error {{it is considered 'const'}}

struct S {
  int m;
  void t() post [p = this] (p != nullptr); // OK
  void u() post [p = this] (++p->m > 0); // expected-error {{cannot assign to variable 'p' with const-qualified type 'const S *'}} expected-note {{variable 'p' declared const here}}
  void v() post [q = ++m] (q > 0); // expected-error {{cannot assign to non-static data member because it is considered 'const' inside of a contract}}
};

int k(int y) post [old = y] (++old > 1);              // OK, not constified
int k2(const int y) post [old = y] (++old > y);       // OK, deduced as int
