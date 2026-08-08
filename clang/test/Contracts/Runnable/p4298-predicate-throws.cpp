// D4298: predicate-evaluation exceptions under noexcept_enforce still
// route through the terminating entry point for the handler call.
// (GCC mirror: g++.dg/contracts/cpp26/p4298-predicate-throws.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <exception>
#include <cstdlib>

bool throws_on_eval(int x) { if (x < 0) throw 1; return x > 0; }

void handle_contract_violation(const std::contracts::contract_violation& v)
{
  if (v.detection_mode() != std::contracts::detection_mode::evaluation_exception)
    __builtin_trap();
  throw 2;  // the handler itself throws
}

int f(int x) pre(throws_on_eval(x)) { return x; }

int main()
{
  std::set_terminate([]() { std::exit(0); });
  f(-1);        // predicate throws -> handler called -> handler throws -> terminate
  __builtin_trap();
}
