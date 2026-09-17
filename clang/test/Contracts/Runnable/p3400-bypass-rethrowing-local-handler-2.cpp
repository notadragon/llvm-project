// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=enforce %libcxx_flags -o %t
// RUN: %t

// P3400: the bypass applies under enforce as well as observe, and to
// all three assertion kinds.  With a rethrowing local handler the exception
// escapes rather than terminating, which is what the caught-and-rethrown code
// did too.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-bypass-rethrowing-local-handler-2.C)

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int global_calls = 0;

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation& v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      throw;
  }
};
constexpr rethrowing_t rethrowing{};

void handle_contract_violation(const contract_violation&) { ++global_calls; }

struct predicate_error {
  int tag;
};

static bool boom() { throw predicate_error{11}; }

int f(int i) pre<rethrowing>(boom()) { return i; }

int p(const int i) post<rethrowing>(r : boom() && r == i) { return i; }

int a(int i) {
  contract_assert<rethrowing>(boom());
  return i;
}

static void expect_throw(int (*fn)(int)) {
  bool caught = false;
  try {
    fn(1);
  } catch (const predicate_error& e) {
    caught = true;
    if (e.tag != 11)
      __builtin_abort();
  }
  if (!caught)
    __builtin_abort();
}

int main() {
  expect_throw(f);
  expect_throw(p);
  expect_throw(a);
  if (global_calls != 0)
    __builtin_abort();
}
