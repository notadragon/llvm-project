// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe \
// RUN:   -fcontract-violation-information=full %libcxx_flags -o %t
// RUN: %t

// -fcontract-violation-information=full, the default, given explicitly:
// every contract-violation record carries the file name, function name,
// line and column, comment and message -- for the cases of
// contract-violation-information-none.cpp, and the compiler does not
// predefine __clang_contracts_violation_information_none.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-full.C.

#ifdef __clang_contracts_violation_information_none
#error "__clang_contracts_violation_information_none is defined"
#endif

#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>
#include <contracts>
#include <cstdlib>
#include <cstring>

using namespace std::contracts;

static int count;
static int local_count;
static assertion_kind expected_kind;
static const char *expected_function;
static unsigned expected_line;
static const char *expected_comment;
static const char *expected_message;

static void check_full(const contract_violation &v) {
  if (std::strcmp(v.location().file_name(), __FILE__) != 0 ||
      !std::strstr(v.location().function_name(), expected_function) ||
      v.location().line() != expected_line || v.location().column() == 0 ||
      std::strcmp(v.comment(), expected_comment) != 0)
    __builtin_abort();
  if (expected_message
          ? !v.message() || std::strcmp(v.message(), expected_message) != 0
          : v.message() != nullptr)
    __builtin_abort();
  if (v.kind() != expected_kind ||
      v.detection_mode() != detection_mode::predicate_false)
    __builtin_abort();
}

void handle_contract_violation(const contract_violation &v) {
  check_full(v);
  if (v.kind() == assertion_kind::cassert) {
    if (v.semantic() != evaluation_semantic::enforce)
      __builtin_abort();
    if (count != 6 || local_count != 1)
      __builtin_abort();
    std::_Exit(0);
  }
  if (v.semantic() != evaluation_semantic::observe)
    __builtin_abort();
  ++count;
}

struct vi_label_t {
  using assertion_control_object = vi_label_t;
  constexpr const char *compute_comment(const char *) const {
    return "computed comment";
  }
  constexpr const char *compute_message(const char *) const {
    return "computed message";
  }
  void handle_contract_violation(const contract_violation &v) const {
    check_full(v);
    ++local_count;
  }
};
constexpr vi_label_t vi_label{};

int pre_f(int x) pre(x > 0) { return x; }            // line 80
int post_f(int x) post(r : r > 0) { return x; }      // line 81
void assert_f(int x) { contract_assert(x > 0); }     // line 82
int div_f(int a, int b) { return a / b; }            // line 83
void msg_f(int x) pre(x > 0, "a user message") {}    // line 84
void label_f(int x) pre<vi_label>(x > 0) {}          // line 85

static void expect(assertion_kind kind, const char *function, unsigned line,
                   const char *comment, const char *message) {
  expected_kind = kind;
  expected_function = function;
  expected_line = line;
  expected_comment = comment;
  expected_message = message;
}

int main() {
  expect(assertion_kind::pre, "pre_f", 80, "x > 0", nullptr);
  pre_f(0);
  // Clang's comment for a postcondition names its result ("r : r > 0").
  expect(assertion_kind::post, "post_f", 81, "r : r > 0", nullptr);
  post_f(0);
  expect(assertion_kind::assert, "assert_f", 82, "x > 0", nullptr);
  assert_f(0);
  expect(assertion_kind::implicit, "div_f", 83, "integer division by zero",
         nullptr);
  div_f(1, 0);
  expect(assertion_kind::pre, "msg_f", 84, "x > 0", "a user message");
  msg_f(0);
  expect(assertion_kind::pre, "label_f", 85, "computed comment",
         "computed message");
  label_f(0);
  if (count != 6 || local_count != 1)
    __builtin_abort();
  expect(assertion_kind::cassert, "main", __LINE__ + 2, "zero == 1", nullptr);
  int zero = 0;
  assert(zero == 1);
  __builtin_abort();
}
