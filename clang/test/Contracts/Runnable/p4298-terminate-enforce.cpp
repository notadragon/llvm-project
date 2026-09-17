// D4298: a throwing handler at noexcept_enforce terminates the program.
//
// Discrimination: a SIGABRT handler prints a marker (checked via the
// process exiting 0 after re-raising is caught) -- mirrors the GCC test's
// intent using set_terminate + exit(0) directly, since the handler throwing
// is exactly what should hit the noexcept boundary and terminate.
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_enforce %libcxx_flags -o %t
// RUN: %t

#include <contracts>
#include <exception>
#include <cstdlib>

void handle_contract_violation(const std::contracts::contract_violation&)
{
  throw 1;
}

int f(int x) pre(x > 0) { return x; }

int main()
{
  std::set_terminate([]() { std::exit(0); });
  f(-1);
  __builtin_trap();  // unreachable: f(-1) must terminate via the handler
}
