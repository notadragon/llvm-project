// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// In a contract predicate, a by-copy capture made by a mutable lambda that
// is itself inside a lambda in the predicate names the inner closure's
// member, whose type is the captured entity's (int), so it can be modified
// ([expr.prim.id.unqual]: the capture rule applies before constification;
// notadragon_wg21 DECISIONS.md E32), also when the enclosing lambda captured
// the entity by reference and so saw it as const.  A namespace-scope
// variable is not captured, so it stays const and stays an error.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/nested-copy-capture-in-predicate.C

int f(int p)
  pre([&] { return [=]() mutable { return ++p; }(); }() > 0) { // ok
  int q = 1;
  contract_assert([&] { return [=]() mutable { return ++q; }(); }() > 0); // ok
  return 0;
}

int g;
void h() {
  contract_assert([&] { return [=]() mutable { return ++g; }(); }() > 0); // expected-error {{cannot assign to variable 'g' because it is considered 'const' inside of a contract}}
}

// The enclosing lambdas may be several, and the outermost may itself copy.
int k(int p) {
  contract_assert([&] { return [&] { return [=]() mutable { return ++p; }(); }(); }() > 0); // ok
  [p] { contract_assert([&] { return [=]() mutable { return ++p; }(); }() > 0); }(); // ok
  contract_assert([&] { return [&] { return ++p; }(); }() > 0); // expected-error {{cannot assign to a variable captured by reference which was captured as const because it is inside a contract}}
  return 0;
}

// A const entity's copy is const.
void c() {
  const int lc = 1; // expected-note {{variable 'lc' declared const here}}
  contract_assert([&] { return [=]() mutable { return ++lc; }(); }() > 0); // expected-error {{cannot assign to}}
}
