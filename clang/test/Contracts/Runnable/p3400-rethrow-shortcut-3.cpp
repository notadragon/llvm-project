// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t

// P3400: a rethrowing local handler reached through delegation -- a helper
// function, and a combined label -- still propagates the predicate's
// exception, whether or not the EH region was elided.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-rethrow-shortcut-8.C)

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;
using std::contracts::violation_handled;
using std::contracts::labels::operator|;

static int global_calls = 0;

void handle_contract_violation(const contract_violation&) { ++global_calls; }

struct predicate_error {
  int tag;
};

static bool boom() { throw predicate_error{5}; }

inline void rethrow_if_exception(const contract_violation& v) {
  if (v.detection_mode() == detection_mode::evaluation_exception)
    throw;
}

// Rethrows through a helper.
struct via_helper_t {
  using assertion_control_object = via_helper_t;
  void handle_contract_violation(const contract_violation& v) const {
    rethrow_if_exception(v);
  }
};

// Rethrows directly, returning violation_handled.
struct rethrows_t {
  using assertion_control_object = rethrows_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw;
    return violation_handled::not_handled;
  }
};

// Only interested in false predicates: does nothing on the exception path.
struct only_predicate_false_t {
  using assertion_control_object = only_predicate_false_t;
  violation_handled handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::predicate_false)
      return violation_handled::handled;
    return violation_handled::not_handled;
  }
};

struct comment_t {
  using assertion_control_object = comment_t;
  constexpr const char* compute_comment(const char* c) const { return c; }
};

constexpr via_helper_t via_helper{};
constexpr rethrows_t rethrows{};
constexpr only_predicate_false_t only_predicate_false{};
constexpr comment_t comment{};

int f_helper(int i) pre<via_helper>(boom()) { return i; }
int f_combined(int i) pre<via_helper | comment>(boom()) { return i; }
int f_second(int i) pre<only_predicate_false | rethrows>(boom()) { return i; }

// A control: the same combined label with a merely false predicate still
// reports through the global handler.
int f_false(int i) pre<via_helper | comment>(i > 0) { return i; }

static void expect_throw(int (*fn)(int)) {
  bool caught = false;
  try {
    fn(1);
  } catch (const predicate_error& e) {
    caught = true;
    if (e.tag != 5)
      __builtin_abort();
  }
  if (!caught)
    __builtin_abort();
}

int main() {
  expect_throw(f_helper);
  expect_throw(f_combined);
  expect_throw(f_second);
  if (global_calls != 0)
    __builtin_abort();

  f_false(-1);
  if (global_calls != 1)
    __builtin_abort();
}
