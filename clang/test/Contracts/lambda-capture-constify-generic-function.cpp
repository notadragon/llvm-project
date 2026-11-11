// RUN: %contracts_verify_test

extern int yy;
void foo(auto p) {
  int local = 202 + p;

  [&](int p2) { // expected-error {{local entity 'p' is used only in contract assertions and must be captured explicitly}} expected-error {{local entity 'local' is used only in contract assertions and must be captured explicitly}} expected-note {{add 'local' to the capture list}} expected-note {{add 'p' to the capture list}}
    contract_assert(p2 && p && local); // expected-note 2 {{contract context}} expected-note 2 {{required here}}
    return p2;
  }(p);

}

int x = (foo(42), 1); // expected-note {{in instantiation of function template specialization 'foo<int>' requested here}}
