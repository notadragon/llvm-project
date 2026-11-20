// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: the rethrowing-local-handler bypass accounts for the contracts of a
// callee it follows.  Calling `rethrow_helper ()' evaluates its
// `pre (log_it ())', which is observable, so the try/catch around the
// predicate stays.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-callee-precondition.C

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int logged = 0;
bool log_it() { ++logged; return true; }
[[noreturn]] void rethrow_helper() pre(log_it()) { throw; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation &v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      rethrow_helper();
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
