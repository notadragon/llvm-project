// D4301: without -fcontracts-p4301 neither the compiler nor the library
// feature-test macro for contract_violation::report() should be defined,
// even though contracts are otherwise enabled.
// (GCC mirror: g++.dg/contracts/cpp26/p4301-report-ftm-undefined.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fsyntax-only %libcxx_flags

#ifdef __cpp_contracts_report
#error "__cpp_contracts_report unexpectedly defined"
#endif

#include <contracts>

#ifdef __cpp_lib_contracts_report
#error "__cpp_lib_contracts_report unexpectedly defined"
#endif

int main() { }
