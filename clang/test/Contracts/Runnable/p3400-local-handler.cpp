// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe && %t 2>&1 | FileCheck %s

#include <contracts>
#include <cstdio>

using namespace std::contracts;
using namespace std::contracts::labels;

int handler_called = 0;

struct my_handler_t {
  using assertion_control_object = my_handler_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    ++handler_called;
    std::printf("local handler called: %s\n", v.comment());
    return violation_handled::handled;
  }
};
constexpr my_handler_t my_handler{};

// CHECK: local handler called:
void f(int x) pre<my_handler>(x > 0) {}

int main() {
  f(-1);
  // CHECK: handler_called=1
  std::printf("handler_called=%d\n", handler_called);
  return 0;
}
