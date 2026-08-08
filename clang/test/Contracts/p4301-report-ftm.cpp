// D4301: verify the compiler and library feature-test macros for
// contract_violation::report() are defined and have the expected value.
// (GCC mirror: g++.dg/contracts/cpp26/p4301-report-ftm.C)
// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4301 -fsyntax-only %libcxx_flags

#ifndef __cpp_contracts_report
#error "__cpp_contracts_report not defined"
#endif
static_assert(__cpp_contracts_report == 202607L);

#include <contracts>

#ifndef __cpp_lib_contracts_report
#error "__cpp_lib_contracts_report not defined"
#endif
static_assert(__cpp_lib_contracts_report == 202607L);

int main() { }
