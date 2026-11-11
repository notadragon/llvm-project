// P3290: a failed assert routes to the contract-violation handler with
// assertion_kind=cassert, detection_mode=predicate_false, and
// evaluation_semantic=enforce, and terminates via std::abort() when the
// handler returns normally (NOT std::terminate()).
//
// The handler validates the violation fields (exiting with a normal, non-crash
// status on mismatch) and returns; the assert must then abort().  A
// std::terminate handler that would _Exit(0) proves abort() -- not
// terminate() -- is used: with `not --crash`, the test passes only if the
// program dies via SIGABRT.
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %libcxx_flags -o %t
// RUN: not --crash %t

#include <cassert>
#include <contracts>
#include <cstdlib>
#include <cstring>
#include <exception>

#ifndef __cpp_lib_assert_can_use_contracts
#  error "__cpp_lib_assert_can_use_contracts not defined"
#endif

using namespace std::contracts;

void handle_contract_violation(const contract_violation& v) {
  if (v.kind() != assertion_kind::cassert)
    std::_Exit(2);
  if (v.detection_mode() != detection_mode::predicate_false)
    std::_Exit(3);
  if (v.semantic() != evaluation_semantic::enforce)
    std::_Exit(4);
  if (!v.comment() || std::strcmp(v.comment(), "1 == 2") != 0)
    std::_Exit(5);
  // Returns normally; the ABI must abort() on completion.
}

[[noreturn]] void my_terminate() { std::_Exit(0); }

int main() {
  std::set_terminate(my_terminate);
  assert(1 == 2);
  return 0;
}
