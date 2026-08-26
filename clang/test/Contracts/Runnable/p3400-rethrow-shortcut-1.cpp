// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t
// RUN: %t
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 \
// RUN:   -fcontract-disable-rethrow-shortcut \
// RUN:   -fcontract-evaluation-semantic=observe %libcxx_flags -o %t.off
// RUN: %t.off

// P3400: a local violation handler that answers an evaluation_exception by
// rethrowing makes the EH region around the predicate pointless -- the front
// end elides it and the exception propagates on its own.  Behaviour must be
// indistinguishable from catching and rethrowing, which is why this runs both
// with and without -fcontract-disable-rethrow-shortcut.
// (GCC mirror: g++.dg/contracts/cpp26/p3400-rethrow-shortcut-1.C)

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

static bool boom() { throw predicate_error{7}; }

// The predicate throws: the exception must come straight out of f.
int f(int i) pre<rethrowing>(boom()) { return i; }

// The predicate is merely false: unchanged path, the local handler declines
// and the global handler reports.
int g(int i) pre<rethrowing>(i > 0) { return i; }

int main() {
  bool caught = false;
  try {
    f(1);
  } catch (const predicate_error& e) {
    caught = true;
    if (e.tag != 7)
      __builtin_abort();
  }
  if (!caught)
    __builtin_abort();
  if (global_calls != 0)
    __builtin_abort();

  // A false predicate still reports normally: only the exception path changed.
  g(-1);
  if (global_calls != 1)
    __builtin_abort();

  // And a satisfied predicate still reports nothing.
  g(1);
  if (global_calls != 1)
    __builtin_abort();
}
