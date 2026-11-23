// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontract-evaluation-semantic=ignore -fsyntax-only -verify %s

// A lambda's postcondition result name cannot have the name of one of the
// lambda's init-captures ([basic.scope.contract]/2: a declaration whose target
// scope is the nearest enclosing lambda scope), beyond the plain case in
// result-name-shadows-lambda-capture.cpp: an init-capture by reference, a
// pack init-capture, a generic lambda, a lambda in a function template.  A
// function declared in a lambda's body has its own parameter scope.  (EDG
// M16b shapes.)  The accepted shapes -- "_", and an outer name used after
// the lambda -- are in result-name-underscore.cpp and
// result-name-lambda-enclosing-scope.cpp.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/result-name-init-capture.C

int v = 0;
auto b = [&r = v](int y) post(r : r > 0) { return y; }; // expected-error {{declaration of result name 'r' shadows lambda capture}} expected-note {{previous declaration is here}}
auto c = [x = 1](auto y) post(x : x > 0) { return y; }; // expected-error {{declaration of result name 'x' shadows lambda capture}} expected-note {{previous declaration is here}}

template <class... T> int pack(T... t) {
  return [... xs = t](int y) post(xs : xs > 0) { return y; }(1); // expected-error {{declaration of result name 'xs' shadows lambda capture}} expected-note {{previous declaration is here}}
}

template <class T> T in_template(T t) {
  return [x = t](T y) post(x : x > 0) { return y; }(t); // expected-error {{declaration of result name 'x' shadows lambda capture}} expected-note {{previous declaration is here}}
}

// OK: a simple capture declares nothing.
void k() {
  int x = 1;
  auto m = [x](int z) post(x : x > 0) { return z; };
}

// OK: a function declared in the body has its own parameter scope.
int l = [x = 1] {
  int f(int) post(x : x > 0);
  return f(x);
}();
