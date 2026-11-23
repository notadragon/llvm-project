// P3290: <cassert> preserves the atypical assert-redefinition behavior -- a
// re-include with a different NDEBUG changes what `assert` expands to, even with
// the contract integration active.  Under NDEBUG the failed assert is a no-op
// (handler not called); without NDEBUG it is active again.
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <cassert>
#include <cstdio>
#include <cstdlib>

// A failed assert calls the handler and then aborts, so the handler reports
// the verdict: reached from the re-enabled assert (success), or from the
// NDEBUG one (failure).
static bool ndebug_done = false;
void handle_contract_violation(const std::contracts::contract_violation&) {
  std::_Exit(ndebug_done ? 0 : 1);
}

#define NDEBUG
#include <cassert>
static void ndebug_call() { assert(1 == 2); }   // no-op under NDEBUG

#undef NDEBUG
#include <cassert>
static void active_call() { assert(1 == 2); }    // active again

int main() {
  ndebug_call();
  ndebug_done = true;
  active_call();
  return 2;   // the re-enabled assert did not call the handler
}
