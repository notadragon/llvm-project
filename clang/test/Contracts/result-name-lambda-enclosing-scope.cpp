// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=enforce -fsyntax-only -verify %s

// A lambda's postcondition result name belongs to the postcondition's own
// scope ([basic.scope.contract]).  It hides an enclosing variable or an
// enclosing function's parameter of the same name only inside the
// postcondition: after the lambda-expression the name means the enclosing
// entity again -- in the lambda's own call arguments and in a later lambda
// capturing it too -- and it conflicts only with the parameters and
// init-captures of the lambda itself, not with the enclosing function's.  A
// function declared at block scope is the same: its result name conflicts
// with its own parameters, not the enclosing function's.  (From the EDG M16b
// shapes.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-lambda-enclosing-scope.C

int q() {
  int r = 1;
  auto l = [](int y) post(r : r > 0) { return y; };
  return r + l(1);
}

int q2() {
  int r = 2;
  auto l = [](int y) post(r : r > 0) { return y; };
  auto m = [&] { return r; };
  return m() + l(1);
}

int q3() {
  int r = 3;
  auto l = [](int y) -> int post(r : r > 0) { return y; };
  return r + l(1);
}

int w = [](int y) {
  int x = y;
  return [x](int z) post(x : x > 0) { return z; }(x);
}(1);

int n = [x = 1] {
  return [](int y) post(x : x > 0) { return y; }(x);
}();

int p(int x) {
  auto l = [](int y) post(x : x > 0) { return y; };
  return l(1) + x;
}

struct S {
  int m(int x) {
    auto l = [](int y) post(x : x > 0) { return y; };
    return l(1) + x;
  }
};

int b(int x) {
  int g(int y) post(x : x > 0);
  return x;
}

// The lambda's own parameter and init-capture still conflict.
int own(int x) {
  auto l = [](int x) -> int post(x : x > 0) { return x; }; // expected-error {{declaration of result name 'x' shadows parameter}} expected-note {{previous declaration is here}}
  auto m = [y = x](int z) -> int post(y : y > 0) { return z; }; // expected-error {{declaration of result name 'y' shadows lambda capture}} expected-note {{previous declaration is here}}
  return 0;
}

// With a namespace-scope variable of the same name in view, the name after
// the lambda must still be the local one, not the namespace-scope one.
constexpr int r = 100;
constexpr int q5() {
  int r = 5;
  auto l = [](int y) post(r : r > 0) { return y; };
  return r + l(1);
}
static_assert(q5() == 6);
