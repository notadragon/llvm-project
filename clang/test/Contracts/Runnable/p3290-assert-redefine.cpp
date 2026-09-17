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

static int hits = 0;
void handle_contract_violation(const std::contracts::contract_violation&) { ++hits; }

#define NDEBUG
#include <cassert>
static void ndebug_call() { assert(1 == 2); }   // no-op under NDEBUG

#undef NDEBUG
#include <cassert>
static void active_call() { assert(1 == 1); }    // active again; passes

int main() {
  ndebug_call();
  if (hits != 0)
    return 1;   // NDEBUG assert must not have called the handler
  active_call();
  return 0;
}
