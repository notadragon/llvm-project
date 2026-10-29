// RUN: %clang_cc1 -std=c++26 -fcontracts -Wno-vla-cxx-extension -fsyntax-only -verify %s

// A VLA named only in a contract_assert inside a default-capturing lambda
// is captured only by the contract assertion: diagnosed, as for any other
// local entity.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/contract-assert-vla-implicit-capture.C

void v(int n) {
  int a[n];
  auto l = [&] { // expected-error {{local entity 'a' is used only in contract assertions and must be captured explicitly}}
    contract_assert(a[0] == 0); // expected-note {{capture of local entity 'a' is required here}} \
                                // expected-note {{within contract context introduced here}}
  };
  // expected-note@-4 {{add 'a' to the capture list}}
  l();
}

// A VLA is handled like any other entity when a nested lambda captures it
// (the capture of the VLA's type, which carries its bound and names no entity,
// takes no part in the rule).  Captured by the nested lambda outside the
// contract assertion: used outside, so not diagnosed.
void nested_outside(int n) {
  int a[n];
  auto l = [&] {
    contract_assert(a[0] == 0);
    [&] { (void)a[0]; }();
  };
  l();
}

// Captured by a nested lambda inside the contract assertion: used only there.
void nested_inside(int n) {
  int a[n];
  auto l = [&] { // expected-error {{local entity 'a' is used only in contract assertions and must be captured explicitly}}
    contract_assert([&] { return a[0] == 0; }()); // expected-note {{capture of local entity 'a' is required here}} \
                                                  // expected-note {{within contract context introduced here}}
  };
  // expected-note@-4 {{add 'a' to the capture list}}
  l();
}

// Captured explicitly: not diagnosed.
void explicit_capture(int n) {
  int a[n];
  auto l = [&a] { contract_assert(a[0] == 0); };
  l();
}

// A use outside the contract assertion that comes after the one inside it
// (GCC-670: GCC's name lookup found the VLA's capture proxy and diagnosed
// these): used outside, so not diagnosed.
void after(int n) {
  int a[n];
  auto l = [&] {
    contract_assert(a[0] == 0);
    (void)a[0];
  };
  l();
}

// The same, followed by two nested lambdas naming the VLA.
void nested_after(int n) {
  int a[n];
  auto l = [&] {
    contract_assert(a[0] == 0);
    [&] { (void)a[0]; }();
    [&] { (void)a[0]; }();
  };
  l();
}

// Named directly and in a lambda within the same predicate: used only in the
// contract assertion.
void nested_inside_after(int n) {
  int a[n];
  auto l = [&] { // expected-error {{local entity 'a' is used only in contract assertions and must be captured explicitly}}
    contract_assert(a[0] == 0 && [&] { return a[0] == 0; }()); // expected-note {{capture of local entity 'a' is required here}} \
                                                               // expected-note {{within contract context introduced here}}
  };
  // expected-note@-4 {{add 'a' to the capture list}}
  l();
}
