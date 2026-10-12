// RUN: %clangxx -std=c++26 %s -DSECOND_TU -c -o %t.2.o
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontract-evaluation-semantic=observe %libcxx_flags %t.2.o -o %t && %t

// P3400: the rethrowing-local-handler bypass does not trust the body of a weak
// callee, which the linker may replace.  Here the weak rethrow_helper is
// `throw;', but the strong definition in the second translation unit (this
// file with -DSECOND_TU) logs first; the handler must run and see it.
//
// GCC: gcc/testsuite/g++.dg/contracts/cpp26/p3400-rethrow-bypass-weak-callee.C

#ifdef SECOND_TU

extern int logged;
[[noreturn]] void rethrow_helper() { ++logged; throw; }

#else

#include <contracts>

using std::contracts::contract_violation;

int logged = 0;
[[gnu::weak, noreturn]] void rethrow_helper() { throw; }

struct rethrowing_t {
  using assertion_control_object = rethrowing_t;
  void handle_contract_violation(const contract_violation &) const {
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

#endif

// REQUIRES: contracts-libcxx, native
