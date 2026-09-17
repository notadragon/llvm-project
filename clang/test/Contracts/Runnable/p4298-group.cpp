// D4298: noexcept_enforce/noexcept_observe are selectable via group config.
// (GCC mirror: g++.dg/contracts/cpp26/p4298-group.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3400 -fcontracts-p4298 -fcontract-group-evaluation-semantic=g:noexcept_observe -fcontract-evaluation-semantic=ignore %libcxx_flags -o %t
// RUN: %t

#include <contracts>

static int fired = 0;
void handle_contract_violation(const std::contracts::contract_violation& v)
{
  ++fired;
  if (v.semantic() != std::contracts::evaluation_semantic::noexcept_observe)
    __builtin_trap();
}

int f(int x) pre<"g"group>(x > 0) { return x; }

int main()
{
  f(-1);
  if (fired != 1) __builtin_trap();
}
