// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe -fsyntax-only

// Test that compute_comment facet compiles with various label types.

#include <contracts>

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
