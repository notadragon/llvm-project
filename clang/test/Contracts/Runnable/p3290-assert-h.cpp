// P3290: the assert integration is equally available when <assert.h> is
// included (rather than <cassert>) -- same behavior: assertion_kind=cassert,
// detection_mode=predicate_false, and a returning handler terminates via
// std::abort() (proved by the set_terminate sentinel + `not --crash`).
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %libcxx_flags -o %t
// RUN: not --crash %t

#include <assert.h>
#include <contracts>
#include <cstdlib>
#include <cstring>
#include <exception>

#ifndef __cpp_lib_assert_can_use_contracts
#  error "__cpp_lib_assert_can_use_contracts not defined via <assert.h>"
#endif

using namespace std::contracts;

void handle_contract_violation(const contract_violation& v) {
  if (v.kind() != assertion_kind::cassert)
    std::_Exit(2);
  if (v.detection_mode() != detection_mode::predicate_false)
    std::_Exit(3);
  // Returns normally; must abort().
}

[[noreturn]] void my_terminate() { std::_Exit(0); }

int main() {
  std::set_terminate(my_terminate);
  assert(1 == 2);
  return 0;
}
