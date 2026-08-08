// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: a local handler that throws propagates out, skipping the global
// handler.  (GCC mirror: g++.dg/contracts/cpp26/p3400-facet-local-exception.C)

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::violation_handled;

static int global_calls = 0;

struct throwing_handler_t {
  using assertion_control_object = throwing_handler_t;
  violation_handled handle_contract_violation(const contract_violation&) const {
    throw 42;
  }
};
constexpr throwing_handler_t throwing_handler{};

void handle_contract_violation(const contract_violation&) {
  ++global_calls;
}

void f(int x) pre<throwing_handler>(x > 0) { }

int main() {
  bool caught = false;
  try {
    f(-1);
  } catch (int e) {
    caught = true;
    if (e != 42) __builtin_abort();
  }
  if (!caught) __builtin_abort();
  if (global_calls != 0) __builtin_abort();
}
