// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -fsyntax-only

// Test that compute_message facet compiles with various label types,
// both with and without explicit P3099 messages.

#include <contracts>

using std::contracts::labels::operator|;
using std::contracts::labels::fixed_message_label_t;

struct redact_message_t {
  using assertion_control_object = redact_message_t;
  constexpr const char* compute_message(const char*) const {
    return "[message redacted]";
  }
};
constexpr redact_message_t redact_message{};

struct passthrough_message_t {
  using assertion_control_object = passthrough_message_t;
  constexpr const char* compute_message(const char* m) const {
    return m;
  }
};
constexpr passthrough_message_t passthrough_message{};

// Without explicit message
void f_no_msg_redact(int x) pre<redact_message>(x > 0) {}
void f_no_msg_passthrough(int x) pre<passthrough_message>(x > 0) {}
void f_no_msg_fixed(int x)
  pre<fixed_message_label_t{"fixed msg"}>(x > 0) {}
void f_no_msg_redact_pass(int x)
  pre<(redact_message | passthrough_message)>(x > 0) {}
void f_no_msg_pass_redact(int x)
  pre<(passthrough_message | redact_message)>(x > 0) {}

// With explicit P3099 message
void f_msg_redact(int x) pre<redact_message>(x > 0, "explicit msg") {}
void f_msg_passthrough(int x)
  pre<passthrough_message>(x > 0, "explicit msg") {}
void f_msg_fixed(int x)
  pre<fixed_message_label_t{"fixed msg"}>(x > 0, "explicit msg") {}
void f_msg_redact_pass(int x)
  pre<(redact_message | passthrough_message)>(x > 0, "explicit msg") {}
void f_msg_pass_redact(int x)
  pre<(passthrough_message | redact_message)>(x > 0, "explicit msg") {}
