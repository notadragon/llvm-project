// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags \
// RUN:   -fsyntax-only

// P3400: contract_control(expr) in various expression contexts.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-control-contexts.C)

#include <contracts>

namespace my_labels {
  struct my_label_t {
    using assertion_control_object = my_label_t;
    constexpr int id() const { return 7; }
  };
  constexpr my_label_t my_label{};
}

using contract_control namespace my_labels;

// 1. Namespace-scope variable initializer.
constexpr auto ns_label = contract_control(my_label);

// 2. Static class member initializer.
struct S {
  static constexpr auto member_label = contract_control(my_label);
  void f(int x) pre<member_label>(x > 0) { }
};

// 3. Template non-type parameter using contract_control.
template<auto Label>
void templated(int x) pre<Label>(x > 0) { }

void use_template() {
  templated<contract_control(my_label)>(1);
}

// 4. static_assert with contract_control.
static_assert(contract_control(my_label).id() == 7);

// 5. constexpr if condition.
void constexpr_if() {
  if constexpr (contract_control(my_label).id() == 7) { }
}

// 6. Default function argument.
constexpr auto default_label() {
  return contract_control(my_label);
}
void with_default(int x, decltype(contract_control(my_label)) label
                  = contract_control(my_label))
  pre<default_label()>(x > 0)
{
}

// 7. Array bound (constexpr context).
constexpr int arr_size = contract_control(my_label).id();
int arr[arr_size];

// 8. Nested contract_control inside an assertion-control expression.
void nested(int x) pre<contract_control(my_label)>(x > 0) { }
