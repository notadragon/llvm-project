// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: the rethrowing-local-handler bypass evaluates the arguments of a
// call it follows, the object of a member call among them: a rethrow reached
// through `rethrow_helper (log_it ())' has a side effect first, so the
// try/catch around the predicate stays and the handler -- with log_it () --
// runs.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-call-arguments.C

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int logged = 0;
int log_it() { ++logged; return 0; }
[[noreturn]] void rethrow_helper(int) { throw; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation &v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      rethrow_helper(log_it());
  }
};
constexpr rethrowing_t rethrowing{};

bool boom() { throw 42; }
int f(int i) pre<rethrowing>(boom()) { return i; }

int main() {
  try { f(1); } catch (int) {}
  if (logged != 1)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
