// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3099: a compile-time-generated (non-literal) diagnostic message given as a
// custom type with .size()/.data() members reaches the handler as text.
// (GCC mirror: g++.dg/contracts/cpp26/p3099-message-cexpr-str.C)
//
// (Previously: a non-string-literal message crashed the parser and, once
// that was fixed, was delivered empty.  Now the message is parsed in a
// constant-evaluated context and its .size()/.data() are extracted like a
// static_assert message.)

#include <contracts>
#include <cstring>

struct MyMessage {
    const char* str;
    constexpr int size() const { return __builtin_strlen(str); }
    constexpr const char* data() const { return str; }
};

static const char* last_message = nullptr;

void handle_contract_violation(const std::contracts::contract_violation& v) {
  last_message = v.message();
}

constexpr MyMessage msg{"custom type message"};

void f(int x) pre(x > 0, msg) { }

int main() {
  f(-1);
  if (!last_message || std::strcmp(last_message, "custom type message") != 0)
    __builtin_abort();
}
