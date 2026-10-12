// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t | FileCheck %s

// Test that compute_comment facet compiles with various label types.
//
// The handler prints the comment each violation reports, which is what
// compute_comment changes.

#include <contracts>

#include <cstdio>

// Every call below violates its precondition.  The handler prints what it
// was given and throws, so each call reports and returns, whatever its
// semantic.
void handle_contract_violation(const std::contracts::contract_violation &v) {
  std::printf("comment \"%s\"\n", v.comment());
  throw 0;
}

template <class F> void call(const char *name, F f) {
  std::printf("%s: ", name);
  try {
    f();
    std::printf("no violation\n");
  } catch (int) {
  }
}

using std::contracts::labels::operator|;

struct redact_comment_t {
  using assertion_control_object = redact_comment_t;
  constexpr const char* compute_comment(const char*) const {
    return "[redacted]";
  }
};
constexpr redact_comment_t redact_comment{};

struct passthrough_comment_t {
  using assertion_control_object = passthrough_comment_t;
  constexpr const char* compute_comment(const char* c) const {
    return c;
  }
};
constexpr passthrough_comment_t passthrough_comment{};

void f_redact(int x) pre<redact_comment>(x > 0) {}
void f_passthrough(int x) pre<passthrough_comment>(x > 0) {}
void f_redact_pass(int x)
  pre<(redact_comment | passthrough_comment)>(x > 0) {}
void f_pass_redact(int x)
  pre<(passthrough_comment | redact_comment)>(x > 0) {}
void f_pass_pass(int x)
  pre<(passthrough_comment | passthrough_comment)>(x > 0) {}

// Empty label (no compute_comment) should also work
void f_empty(int x) pre<empty_label>(x > 0) {}

// Combined with empty
void f_redact_empty(int x)
  pre<(redact_comment | empty_label)>(x > 0) {}
void f_empty_redact(int x)
  pre<(empty_label | redact_comment)>(x > 0) {}

int main() {
  call("f_redact", [] { f_redact(-1); });
  call("f_passthrough", [] { f_passthrough(-1); });
  call("f_redact_pass", [] { f_redact_pass(-1); });
  call("f_pass_redact", [] { f_pass_redact(-1); });
  call("f_pass_pass", [] { f_pass_pass(-1); });
  call("f_empty", [] { f_empty(-1); });
  call("f_redact_empty", [] { f_redact_empty(-1); });
  call("f_empty_redact", [] { f_empty_redact(-1); });
}

// CHECK: f_redact: comment "[redacted]"
// CHECK-NEXT: f_passthrough: comment "x > 0"
// CHECK-NEXT: f_redact_pass: comment "[redacted]"
// CHECK-NEXT: f_pass_redact: comment "[redacted]"
// CHECK-NEXT: f_pass_pass: comment "x > 0"
// CHECK-NEXT: f_empty: comment "x > 0"
// CHECK-NEXT: f_redact_empty: comment "[redacted]"
// CHECK-NEXT: f_empty_redact: comment "[redacted]"

// REQUIRES: contracts-libcxx, native
