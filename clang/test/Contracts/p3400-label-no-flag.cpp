// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify %s

// Without -fcontracts-p3400, label syntax is rejected.

struct L { using assertion_control_object = L; };
constexpr L l{};

void f(int x) pre<l>(x > 0) {} // expected-error {{expected '(' after 'pre'}} expected-error {{use of undeclared identifier 'x'}} expected-error {{expected ';' after top level declarator}}
