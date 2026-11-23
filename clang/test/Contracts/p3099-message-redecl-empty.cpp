// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3099 -fsyntax-only -verify %s

// P3099: an empty diagnostic message is a message, distinct from none
// (P3099R3 sec. "Empty string vs. no string": message() is "" for the one
// and null for the other).  So a redeclaration that adds or drops an empty
// message is mismatched, in either order, while two empty messages match --
// including an empty string literal and a user-defined message object whose
// extracted text is empty, since sameness is decided on the extracted text.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3099-message-redecl-empty.C

struct Empty {
  constexpr int size() const { return 0; }
  constexpr const char *data() const { return ""; }
};

void f(int x) pre(x > 0, ""); // expected-note {{contract previously specified with a different diagnostic message}}
void f(int x) pre(x > 0); // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}

void g(int x) pre(x > 0); // expected-note {{contract previously specified with a different diagnostic message}}
void g(int x) pre(x > 0, ""); // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}

void h(int x) pre(x > 0, Empty{}); // expected-note {{contract previously specified with a different diagnostic message}}
void h(int x) pre(x > 0); // expected-error {{differs in contract specifier sequence}} expected-note {{in contract specified here}}

void i(int x) pre(x > 0, "");
void i(int x) pre(x > 0, ""); // OK

void j(int x) pre(x > 0, "");
void j(int x) pre(x > 0, Empty{}); // OK, the same (empty) text
