// RUN: %clang_cc1 -std=c++26 -fcontracts  -fsyntax-only -verify %s

// Smoke test: a contract_assert inside a generic lambda, naming a local and
// the lambda's own parameter.  Small on purpose -- it is the first thing to
// check when contract parsing inside a lambda regresses.
extern int yy;
auto tmpl_lambda = [](auto p) {
  int local = 202 + p;

  return [&](int p2) { // expected-error {{local entity 'p' is used only in contract assertions and must be captured explicitly}} expected-error {{local entity 'local' is used only in contract assertions and must be captured explicitly}} expected-note {{add 'local' to the capture list}} expected-note {{add 'p' to the capture list}}
    contract_assert(p2 && p && local); // expected-note 2 {{contract context}} expected-note 2 {{required here}}
    return p2;
  }(p);

}(yy); // expected-note {{in instantiation of function template specialization '(lambda)::operator()<int>' requested here}}
