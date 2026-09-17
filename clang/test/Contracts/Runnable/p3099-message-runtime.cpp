// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3099 %libcxx_flags -o %t -fcontract-evaluation-semantic=observe
// RUN: %t

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

void no_message(int x) pre(x > 0) {}

void with_message(int x) pre(x > 0, "x must be positive") {}

int with_post(int x) post(r: r >= 0, "non-negative result") { return x; }

void with_assert(int x) {
  contract_assert(x != 0, "x must not be zero");
}

int main() {
  // Test 1: no message — comment is source text, message is null
  expected_comment = "x > 0";
  expected_message = nullptr;
  no_message(-1);

  // Test 2: syntactic message — comment is source text, message is user text
  expected_comment = "x > 0";
  expected_message = "x must be positive";
  with_message(-1);

  // Test 3: postcondition with message — comment includes result name
  expected_comment = "r: r >= 0";
  expected_message = "non-negative result";
  with_post(-1);

  // Test 4: contract_assert with message
  expected_comment = "x != 0";
  expected_message = "x must not be zero";
  with_assert(0);

  std::fprintf(stderr, "All %d tests passed\n", test_num);
  return 0;
}
