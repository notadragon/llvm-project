// RUN: %clangxx -std=c++26 %s -fcontracts %libcxx_flags -o %t -fcontract-evaluation-semantic=observe
// RUN: %t

// Verify that [[clang::contract_message]] populates message(), not comment().
// No -fcontracts-p3099 needed for attribute usage.

#include <contracts>
#include <cstdio>
#include <cstring>
#include <cstdlib>

using std::contracts::contract_violation;

int test_num = 0;
const char* expected_comment = nullptr;
const char* expected_message = nullptr;

void handle_contract_violation(const contract_violation& v) {
  ++test_num;
  bool comment_ok = std::strcmp(v.comment(), expected_comment) == 0;
  // P3099: message() is nullptr when no message was supplied (expected_message
  // == nullptr), and points at the text otherwise.
  bool message_ok = expected_message == nullptr
                        ? v.message() == nullptr
                        : v.message() != nullptr &&
                              std::strcmp(v.message(), expected_message) == 0;
  if (!comment_ok) {
    std::fprintf(stderr, "FAIL test %d: comment expected '%s', got '%s'\n",
                 test_num, expected_comment, v.comment());
    std::abort();
  }
  if (!message_ok) {
    std::fprintf(stderr, "FAIL test %d: message expected '%s', got '%s'\n",
                 test_num, expected_message, v.message());
    std::abort();
  }
  std::fprintf(stderr, "PASS test %d\n", test_num);
}

void attr_message(int x)
  pre [[clang::contract_message("must be positive")]] (x > 0) {}

void no_attr(int x) pre(x > 0) {}

int main() {
  // Test 1: attribute message goes to message(), comment() is source text
  expected_comment = "x > 0";
  expected_message = "must be positive";
  attr_message(-1);

  // Test 2: no attribute — message is null, comment is source text
  expected_comment = "x > 0";
  expected_message = nullptr;
  no_attr(-1);

  std::fprintf(stderr, "All %d tests passed\n", test_num);
  return 0;
}
