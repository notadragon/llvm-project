// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s

// Names from contract_control using-directives are NOT visible
// in regular code — only in assertion-control expressions.

namespace my_labels {
  struct my_label_t {
    using assertion_control_object = my_label_t;
  };
  constexpr my_label_t my_label{};
  constexpr int value = 42; // expected-note {{'::value' declared here}}
}

using contract_control namespace my_labels;

// Should NOT find 'value' in regular code:
int x = value; // expected-error {{use of undeclared identifier 'value'}}

// Should NOT find 'my_label' in regular code:
auto bad = my_label; // expected-error {{use of undeclared identifier 'my_label'}}

// But fully-qualified access works:
auto good = my_labels::my_label;

// In label expressions, contract_control names ARE visible:
void f(int x) pre<my_label>(x > 0) {}
