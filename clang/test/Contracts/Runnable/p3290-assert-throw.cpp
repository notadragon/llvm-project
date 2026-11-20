// P3290: when a failed assert's contract-violation handler exits via an
// exception, the assert entry point catches it and terminates via
// std::abort() (NOT std::terminate()).  This is specific to the C assert
// integration: the paper requires abort() on *any* completion of the handler,
// so the entry point wraps the dispatch in try { ... } catch { std::abort(); }.
//
// A std::terminate handler that would _Exit(0) proves abort() is used: with
// `not --crash`, the test passes only if the program dies via SIGABRT.
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -D__STDC_WANT_ASSERT_USES_CONTRACTS__ %libcxx_flags -o %t
// RUN: not --crash %t

#include <cassert>
#include <contracts>
#include <cstdlib>
#include <exception>

using namespace std::contracts;

void handle_contract_violation(const contract_violation& v) {
  if (v.kind() != assertion_kind::cassert)
    std::_Exit(2);
  throw 42;
}

[[noreturn]] void my_terminate() { std::_Exit(0); }

int main() {
  std::set_terminate(my_terminate);
  assert(1 == 2);
  return 0;
}
