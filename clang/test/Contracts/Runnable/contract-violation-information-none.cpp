// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3850 \
// RUN:   -fcontract-evaluation-semantic=observe \
// RUN:   -fcontract-group-evaluation-semantic=ub:expr.mul.div.by.zero:observe \
// RUN:   -fcontract-violation-information=none %libcxx_flags -o %t
// RUN: %t

// -fcontract-violation-information=none: every contract-violation record
// the compiler emits carries an empty file name, an empty function name,
// line 0, column 0, an empty comment and no message -- for a precondition,
// a postcondition, a contract_assert, an implicit (P3100) contract
// assertion, a contract with a user message (P3099), a label's
// compute_comment and compute_message results (P3400), and the
// contract-checking assert of <cassert> (P3290).  The kind, semantic,
// detection mode and the label's local handler are unchanged, and the
// compiler predefines __clang_contracts_violation_information_none.
// contract-violation-information-full.cpp is the same program under =full.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/g++.dg/contracts/cpp26/contract-violation-information-none.C.

#ifndef __clang_contracts_violation_information_none
#error "__clang_contracts_violation_information_none is not defined"
#endif

#define __STDC_WANT_ASSERT_USES_CONTRACTS__ 1
#include <cassert>
#include <contracts>
#include <cstdlib>

using namespace std::contracts;

static int count;
static int local_count;
static assertion_kind expected_kind;
static detection_mode expected_mode = detection_mode::predicate_false;

static void check_empty(const contract_violation &v) {
  if (v.location().file_name()[0] != '\0' ||
      v.location().function_name()[0] != '\0' || v.location().line() != 0 ||
      v.location().column() != 0 || v.comment()[0] != '\0' ||
      v.message() != nullptr)
    __builtin_abort();
  if (v.kind() != expected_kind || v.detection_mode() != expected_mode)
    __builtin_abort();
}

void handle_contract_violation(const contract_violation &v) {
  check_empty(v);
  if (v.kind() == assertion_kind::cassert) {
    // The <cassert> path enforces: it terminates after the handler.
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

// A label whose computed comment and message would otherwise be recorded,
// with a local handler that still runs.
struct vi_label_t {
  using assertion_control_object = vi_label_t;
  constexpr const char *compute_comment(const char *) const {
    return "computed comment";
  }
  constexpr const char *compute_message(const char *) const {
    return "computed message";
  }
  void handle_contract_violation(const contract_violation &v) const {
    check_empty(v);
    ++local_count;
  }
};
constexpr vi_label_t vi_label{};

int pre_f(int x) pre(x > 0) { return x; }
int post_f(int x) post(r : r > 0) { return x; }
void assert_f(int x) { contract_assert(x > 0); }
int div_f(int a, int b) { return a / b; }
void msg_f(int x) pre(x > 0, "a user message") {}
void label_f(int x) pre<vi_label>(x > 0) {}

int main() {
  expected_kind = assertion_kind::pre;
  pre_f(0);
  expected_kind = assertion_kind::post;
  post_f(0);
  expected_kind = assertion_kind::assert;
  assert_f(0);
  expected_kind = assertion_kind::implicit;
  div_f(1, 0);
  expected_kind = assertion_kind::pre;
  msg_f(0);
  label_f(0);
  if (count != 6 || local_count != 1)
    __builtin_abort();
  expected_kind = assertion_kind::cassert;
  int zero = 0;
  assert(zero == 1);
  __builtin_abort();
}
