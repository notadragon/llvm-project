// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A contract assertion in a generic lambda follows the capture rules a
// non-generic lambda's does: an entity odr-used only in contract assertions
// is not implicitly captured, one used outside them as well is, and an
// explicit capture is always fine.  A constified name in the lambda's body
// used to be skipped when the potential captures were resolved, so the
// capture was never made and instantiation failed ("reference to local
// variable declared in enclosing function"), for the valid cases too.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/generic-lambda-contract-capture.C

int only(int x) {
  auto l = [&](auto a) { contract_assert(x > a); return a; };
  // expected-error@-1 {{local entity 'x' is used only in contract assertions and must be captured explicitly}}
  // expected-note@-2 {{capture of local entity 'x' is required here}}
  // expected-note@-3 {{within contract context introduced here}}
  // expected-note@-4 {{add 'x' to the capture list}}
  return l(1);
}

int also_outside(int x) {
  auto l = [&](auto a) { contract_assert(x > a); return a + x; };
  return l(1);
}

int explicit_capture(int x) {
  auto l = [&x](auto a) { contract_assert(x > a); return a; };
  return l(1);
}

// In a predicate, a generic lambda captures the parameter or result name it
// names.
int in_pre(int x) pre([&](auto k) { return x + k; }(1) > 0) { return x; }
int in_post() post(r: [&](auto k) { return r + k; }(1) > 0) { return 5; }
