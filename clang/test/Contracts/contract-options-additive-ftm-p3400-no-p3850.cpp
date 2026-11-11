// RUN: %clangxx -std=c++26 -fcontracts-p3400 -fno-contracts-p3850 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// DECISIONS.md E175: -fno-contracts-p3850 does not turn off the P4451 that
// an explicit -fcontracts-p3400 implies, so __cpp_contracts and
// __cpp_lib_contracts are raised whenever P3400 is on, and P3400's library
// is the one declared.  The contract-options-additive-*.cpp tests cover the
// other combinations in C++23.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-ftm-p3400-no-p3850.C

#include <contracts>

#if !(__cpp_contracts > 202502L)
#error "__cpp_contracts"
#endif
#if !(__cpp_lib_contracts > 202502L)
#error "__cpp_lib_contracts"
#endif
#if !(__clang_contracts_p4451 > 0)
#error "__clang_contracts_p4451"
#endif
#ifndef __cpp_lib_contracts_labels
#error "__cpp_lib_contracts_labels"
#endif

constexpr auto &t = std::contracts::labels::terminating;
constexpr auto &e = std::contracts::labels::empty_label;
void f(int x) pre<terminating>(x > 0);
