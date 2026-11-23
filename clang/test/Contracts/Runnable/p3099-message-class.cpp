// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: messages work on contracts in class member functions (the deferred
// parsing path).  (GCC mirror: g++.dg/contracts/cpp26/p3099-message-class.C)

#include <contracts>
#include <cstring>

static const char* last_message = nullptr;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_message = v.message();
}

struct Widget {
  void set_value(int x) pre(x > 0, "value must be positive") { val = x; }
  int get_value() const
    post(r: r >= 0, "cached value must be non-negative") { return val; }
  int val = -1;
};

int main() {
  Widget w;
  w.set_value(-1);
  if (!last_message || std::strcmp(last_message, "value must be positive") != 0)
    __builtin_abort();
}
