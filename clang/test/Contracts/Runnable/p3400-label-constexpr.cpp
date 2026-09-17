// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: various constexpr forms for assertion-control labels.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-label-constexpr.C)

#include <contracts>

struct my_label_t {
  using assertion_control_object = my_label_t;
};

constexpr my_label_t my_label{};
constexpr my_label_t get_label() { return my_label_t{}; }

// Direct constructor invocation as the label expression.
void f(int x) pre<my_label_t{}>(x > 0) { }
// Constexpr variable.
void g(int x) pre<my_label>(x > 0) { }
// Constexpr function call.
void h(int x) pre<get_label()>(x > 0) { }
// Library-provided empty_label.
void k(int x) pre<std::contracts::labels::empty_label>(x > 0) { }
// contract_assert with constexpr function.
void m() { contract_assert<get_label()>(true); }
// Postcondition with constexpr variable.
int n(int x) post<my_label>(r: r >= 0) { return x; }

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++handler_count;
}

int main() {
  f(-1);
  if (handler_count != 1) __builtin_abort();
  g(-1);
  if (handler_count != 2) __builtin_abort();
  h(-1);
  if (handler_count != 3) __builtin_abort();
  k(-1);
  if (handler_count != 4) __builtin_abort();
  m();  // passes
  n(-1);
  if (handler_count != 5) __builtin_abort();
}
