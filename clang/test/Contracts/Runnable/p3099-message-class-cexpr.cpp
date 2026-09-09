// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: a custom-type (non-literal) diagnostic message on a member-function
// postcondition/precondition is delivered to the handler.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-class-cexpr.C.)
//
// (Previously: a non-string-literal message crashed the parser / was
// delivered empty; now parsed in a constant-evaluated context and its
// .size()/.data() extracted like a static_assert message.)

#include <contracts>
#include <cstring>

struct MyMessage {
  const char* str;
  constexpr int size() const { return __builtin_strlen(str); }
  constexpr const char* data() const { return str; }
};
constexpr MyMessage msg{"member custom msg"};

static const char* last = nullptr;
void handle_contract_violation(const std::contracts::contract_violation& v) {
  last = v.message();
}

struct Widget {
  int val = -1;
  void set_value(int x) pre(x > 0, msg) { val = x; }
  int get_value() const post(r: r >= 0, msg) { return val; }
};

int main() {
  Widget w;
  w.set_value(-1);
  if (!last || std::strcmp(last, "member custom msg") != 0) __builtin_abort();
}
