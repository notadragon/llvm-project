// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// Basic: using contract_control namespace makes labels available
// unqualified in assertion-control expressions.

namespace my_labels {
  struct my_label_t {
    using assertion_control_object = my_label_t;
  };
  constexpr my_label_t my_label{};
}

using contract_control namespace my_labels;

// my_label is visible in label expression but not in regular code.
void f(int x) pre<my_label>(x > 0) {}

int g(int x) post<my_label>(r: r >= 0) { return x; }

void h() {
  contract_assert<my_label>(true);
}

// Fully-qualified access still works alongside contract_control.
void k(int x) pre<my_labels::my_label>(x > 0) {}
