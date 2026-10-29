// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: the rethrowing-local-handler bypass accounts for the local handler's
// own contracts.  The handler's body is `throw;', but calling it evaluates its
// `pre (log_it ())', so the try/catch around the predicate stays.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-handler-precondition.C

#include <contracts>

using std::contracts::contract_violation;

static int logged = 0;
bool log_it() { ++logged; return true; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation &) const
      pre(log_it()) { throw; }
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
