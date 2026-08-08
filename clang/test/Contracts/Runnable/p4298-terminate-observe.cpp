// D4298: a throwing handler at noexcept_observe terminates the program
// (unlike plain observe, where a throwing handler would simply propagate).
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=noexcept_observe %libcxx_flags -o %t
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
