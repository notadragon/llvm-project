// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// contract_control(expr) evaluates expr with augmented lookup active,
// for declaring labels outside of assertion-control expressions.

namespace my_labels {
  struct my_label_t {
    using assertion_control_object = my_label_t;
  };
  constexpr my_label_t my_label{};
}

using contract_control namespace my_labels;

// contract_control(expr) makes the name available:
constexpr auto label_copy = contract_control(my_label);

void f(int x) pre<label_copy>(x > 0) {}

// Can also use contract_control in variable initialization:
constexpr auto another = contract_control(my_label_t{});
void g(int x) pre<another>(x > 0) {}
