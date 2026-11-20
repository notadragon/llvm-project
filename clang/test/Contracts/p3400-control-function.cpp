// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p3400 -fsyntax-only -verify %s
// expected-no-diagnostics

// Function-scope contract_control using-directive.

namespace my_labels {
  struct my_label_t {
    using assertion_control_object = my_label_t;
  };
  constexpr my_label_t my_label{};
}

void f(int x)
  pre<my_labels::my_label>(x > 0)
{
  using contract_control namespace my_labels;
  contract_assert<my_label>(x > 0);
}

// Multiple contract_control directives coexisting.
namespace other_labels {
  struct other_t {
    using assertion_control_object = other_t;
  };
  constexpr other_t other{};
}

void g(int x) {
  using contract_control namespace my_labels;
  using contract_control namespace other_labels;
  contract_assert<my_label>(x > 0);
  contract_assert<other>(x > 0);
}
