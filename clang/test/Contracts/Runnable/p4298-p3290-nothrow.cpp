// D4298 + P3290: the nothrow_t overloads report noexcept_enforce /
// noexcept_observe, not plain enforce/observe, when P3290 is enabled.
// This is unconditional (whenever __cpp_lib_contracts_api is on), matching
// the GCC implementation and D4298R0's wording.
// (GCC mirror: g++.dg/contracts/cpp26/p4298-p3290-nothrow.C)
//
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3290 -fcontracts-p4298 %libcxx_flags -o %t
// RUN: %t

#include <contracts>
using namespace std::contracts;

static evaluation_semantic seen = evaluation_semantic::unspecified;
void handle_contract_violation(const contract_violation& v) { seen = v.semantic(); }

int main()
{
  handle_observed_contract_violation(std::nothrow, "msg");
  if (seen != evaluation_semantic::noexcept_observe)
    __builtin_trap();
}
