// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p3099 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t | FileCheck %s

// Test that compute_message facet compiles with various label types,
// both with and without explicit P3099 messages.
//
// The handler prints the message each violation reports, which is what
// compute_message changes.

#include <contracts>
#include <cstdio>

// Every call below violates its precondition, and observe lets it return.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  const char *m = v.message();
  std::printf("message \"%s\"\n", m ? m : "(null)");
}

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

int main() {
  std::printf("f_no_msg_redact: ");
  f_no_msg_redact(-1);
  std::printf("f_no_msg_passthrough: ");
  f_no_msg_passthrough(-1);
  std::printf("f_no_msg_fixed: ");
  f_no_msg_fixed(-1);
  std::printf("f_no_msg_redact_pass: ");
  f_no_msg_redact_pass(-1);
  std::printf("f_no_msg_pass_redact: ");
  f_no_msg_pass_redact(-1);
  std::printf("f_msg_redact: ");
  f_msg_redact(-1);
  std::printf("f_msg_passthrough: ");
  f_msg_passthrough(-1);
  std::printf("f_msg_fixed: ");
  f_msg_fixed(-1);
  std::printf("f_msg_redact_pass: ");
  f_msg_redact_pass(-1);
  std::printf("f_msg_pass_redact: ");
  f_msg_pass_redact(-1);
}

// CHECK: f_no_msg_redact: message "[message redacted]"
// CHECK-NEXT: f_no_msg_passthrough: message "(null)"
// CHECK-NEXT: f_no_msg_fixed: message "fixed msg"
// CHECK-NEXT: f_no_msg_redact_pass: message "[message redacted]"
// CHECK-NEXT: f_no_msg_pass_redact: message "[message redacted]"
// CHECK-NEXT: f_msg_redact: message "[message redacted]"
// CHECK-NEXT: f_msg_passthrough: message "explicit msg"
// CHECK-NEXT: f_msg_fixed: message "fixed msg"
// CHECK-NEXT: f_msg_redact_pass: message "[message redacted]"
// CHECK-NEXT: f_msg_pass_redact: message "[message redacted]"

// REQUIRES: contracts-libcxx, native
