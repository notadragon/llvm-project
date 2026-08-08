// D4298: noexcept_enforce is selectable via JSON configuration.
// (GCC mirror: g++.dg/contracts/cpp26/p4298-json-config.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4298 -fcontract-evaluation-semantic=ignore -fcontract-configuration-file=%S/p4298-json-config.json %libcxx_flags -o %t
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
  __builtin_trap();
}
