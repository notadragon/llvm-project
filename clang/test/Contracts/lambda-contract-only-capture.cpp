// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// A local entity implicitly captured by a lambda only because it is named in
// contract assertions makes the program ill-formed ([expr.prim.lambda.capture]),
// including a capture made on behalf of a lambda inside the predicate (the
// standard's example f7), and in a generic lambda whose predicate is
// type-dependent, which has no other statement to commit the capture.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/lambda-contract-only-capture.C,
// lambda-contract-only-capture-generic.C and
// lambda-contract-only-capture-nested.C in the gnu_gcc fork.

struct S {
  int m;
  void g() {
    auto s1 = [&] pre(m > 0) {};               // expected-error {{'this' is used only in contract assertions}}
    // expected-note@-1 {{capture of 'this' is required here}}
    // expected-note@-2 {{within contract context introduced here}}
    // expected-note@-3 {{add 'this' to the capture list}}
    auto s2 = [&] { contract_assert(m > 0); }; // expected-error {{'this' is used only in contract assertions}}
    // expected-note@-1 {{capture of 'this' is required here}}
    // expected-note@-2 {{within contract context introduced here}}
    // expected-note@-3 {{add 'this' to the capture list}}
  }
};

void h() {
  bool y = true;
  auto n1 = [=] pre([=] { return y; }()) {};   // expected-error {{local entity 'y' is used only in contract assertions}}
  // expected-note@-1 {{capture of local entity 'y' is required here}}
  // expected-note@-2 {{within contract context introduced here}}
  // expected-note@-3 {{add 'y' to the capture list}}
}

void k(int x) {
  auto c = [=] pre(x > 0) {};                  // expected-error {{local entity 'x' is used only in contract assertions}}
  // expected-note@-1 {{capture of local entity 'x' is required here}}
  // expected-note@-2 {{within contract context introduced here}}
  // expected-note@-3 {{add 'x' to the capture list}}
}

void gen(int t) {
  auto t1 = [=](auto u) pre(u > t) {};         // expected-error {{local entity 't' is used only in contract assertions}}
  // expected-note@-1 {{capture of local entity 't' is required here}}
  // expected-note@-2 {{within contract context introduced here}}
  // expected-note@-3 {{add 't' to the capture list}}
  t1(1);
  auto t2 = [&](auto u) { contract_assert(u > t); }; // expected-error {{local entity 't' is used only in contract assertions}}
  // expected-note@-1 {{capture of local entity 't' is required here}}
  // expected-note@-2 {{within contract context introduced here}}
  // expected-note@-3 {{add 't' to the capture list}}
  t2(1);
}
