// Mirror of g++.dg/contracts/cpp26/contract-capture-note-location.C.
//
// A contract assertion may not cause a capture that exists only for it, and
// the diagnostic for that carries notes.  On the GCC side the note's location
// was read out of the capture's initializer, which stops being a DECL once
// the capture crosses two lambdas, giving a garbage line number (and a
// tree-check ICE in a checking build).
//
// Clang has never had that bug -- it does not build the note from the
// capture initializer at all.  This test exists as a regression pin: every
// diagnostic below is anchored to the line it belongs on, so a location that
// went wrong would stop matching.  Two and three levels of nesting are
// covered because a one-level-only fix would still be wrong at three, and
// generic lambdas are covered because the original upstream report blamed
// them and they are in fact irrelevant to the GCC bug.  In the generic case
// the function's local and the outer lambda's parameter both get the
// contract-capture diagnostic, each with its notes on its own line.
//
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

void one_level() {
  int x = 1;
  auto l = [&]() { contract_assert(x > 0); };
  // expected-error@-1 {{local entity 'x' is used only in contract assertions and must be captured explicitly}}
  // expected-note@-2 {{capture of local entity 'x' is required here}}
  // expected-note@-3 {{within contract context introduced here}}
  // expected-note@-4 {{add 'x' to the capture list}}
  l();
}

void two_levels() {
  int x = 1;
  auto outer = [&]() {
    auto inner = [&]() { contract_assert(x > 0); };
    // expected-error@-1 {{local entity 'x' is used only in contract assertions and must be captured explicitly}}
    // expected-note@-2 {{capture of local entity 'x' is required here}}
    // expected-note@-3 {{within contract context introduced here}}
    // expected-note@-4 {{add 'x' to the capture list}}
    inner();
  };
  outer();
}

void three_levels() {
  int x = 1;
  auto a = [&]() {
    auto b = [&]() {
      auto c = [&]() { contract_assert(x > 0); };
      // expected-error@-1 {{local entity 'x' is used only in contract assertions and must be captured explicitly}}
      // expected-note@-2 {{capture of local entity 'x' is required here}}
      // expected-note@-3 {{within contract context introduced here}}
      // expected-note@-4 {{add 'x' to the capture list}}
      c();
    };
    b();
  };
  a();
}

void two_levels_generic() {
  int x = 1;
  auto outer = [&](auto f) {
    auto inner = [&](auto g) { contract_assert(x + f + g > 0); };
    // expected-error@-1 {{local entity 'x' is used only in contract assertions and must be captured explicitly}}
    // expected-error@-2 {{local entity 'f' is used only in contract assertions and must be captured explicitly}}
    // expected-note@-3 {{capture of local entity 'x' is required here}}
    // expected-note@-4 {{capture of local entity 'f' is required here}}
    // expected-note@-5 2 {{within contract context introduced here}}
    // expected-note@-6 {{add 'x' to the capture list}}
    // expected-note@-7 {{add 'f' to the capture list}}
    inner(1);
  };
  outer(1);
  // expected-note@-1 {{in instantiation of function template specialization}}
}
