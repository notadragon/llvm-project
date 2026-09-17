// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -fsyntax-only

// operator| found solely through <contracts> header's
// using contract_control namespace directive.

#include <contracts>

struct my_label_t {
  using assertion_control_object = my_label_t;
};
constexpr my_label_t my_label{};

// operator| is visible in assertion-control expressions via the
// header's using contract_control namespace std::contracts::labels;
void f(int x) pre<my_label | empty_label>(x > 0) {}
void g(int x) pre<empty_label | my_label>(x > 0) {}
void h(int x) pre<empty_label | empty_label>(x > 0) {}

// Chaining
void k(int x) pre<my_label | empty_label | my_label>(x > 0) {}
