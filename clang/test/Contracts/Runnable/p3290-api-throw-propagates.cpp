// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 %libcxx_flags -o %t
// RUN: %t

// P3290: the *non-nothrow* API overloads let a throwing violation handler's
// exception propagate to the caller -- the behavior the std::nothrow_t overloads
// (which terminate instead, see p3290-api-nothrow.cpp) exist to opt out of.
// Covers both handle_observed_contract_violation and the [[noreturn]] throwing
// handle_enforced_contract_violation, whose abort() backstop is reached only on
// a *normal* handler return, not on a throw.
// (GCC mirror: g++.dg/contracts/cpp26/p3290-api-throw-propagates.C.)

#include <contracts>

void handle_contract_violation(const std::contracts::contract_violation&) {
  throw 42;
}

int main() {
  int caught = 0;

  // observe: non-nothrow, so a throwing handler propagates (does not terminate).
  try {
    std::contracts::handle_observed_contract_violation("observe throws");
  } catch (int e) {
    if (e == 42)
      ++caught;
  }

  // enforce: [[noreturn]] but non-nothrow; a throwing handler propagates rather
  // than hitting the abort() backstop (which is only for a normal return).
  try {
    std::contracts::handle_enforced_contract_violation("enforce throws");
  } catch (int e) {
    if (e == 42)
      ++caught;
  }

  if (caught != 2)
    __builtin_abort();
}
