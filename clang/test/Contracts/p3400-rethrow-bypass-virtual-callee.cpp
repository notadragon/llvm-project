// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t

// P3400: the rethrowing-local-handler bypass does not follow a virtual call to
// its static callee.  The handler calls `g_base.rethrow ()'; Base::rethrow is
// `throw;', but the dynamic overrider Logger::rethrow logs first, so the
// handler has an observable effect and the try/catch around the predicate
// stays.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-virtual-callee.C

#include <contracts>

using std::contracts::contract_violation;
using std::contracts::detection_mode;

static int logged = 0;
struct Base { [[noreturn]] virtual void rethrow() const { throw; } };
struct Logger : Base {
  [[noreturn]] void rethrow() const override { ++logged; throw; }
};
const Logger the_logger;
const Base &g_base = the_logger;

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation &v) const {
    if (v.detection_mode() == detection_mode::evaluation_exception)
      g_base.rethrow();
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
