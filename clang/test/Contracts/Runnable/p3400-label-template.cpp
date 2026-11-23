// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: a function template whose precondition and postcondition carry a
// variable-template label naming the template parameter (the closing >>
// split); each instantiation is checked at run time.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-label-template.C

#include <contracts>

struct empty_label_t {
  using assertion_control_object = empty_label_t;
};

template <typename T>
constexpr empty_label_t typed_label{};

template <typename T>
void check(T x) pre<typed_label<T>>(x > T{}) {}

template <typename T>
T positive(T x) post<typed_label<T>>(r: r > T{}) { return x; }

static int handler_count = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++handler_count;
}

int main() {
  check(1);    // OK
  check(-1);   // Violates
  check(2.0);  // OK
  if (handler_count != 1) __builtin_abort();

  positive(5);    // OK
  positive(-1);   // Violates
  positive(-1L);  // Violates
  if (handler_count != 3) __builtin_abort();
}
