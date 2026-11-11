// RUN: %clangxx -std=c++26 -fcontracts-p3850 -fno-contracts-p3400 %libcxx_flags -fsyntax-only -Xclang -verify %s
// RUN: %clangxx -std=c++26 -fno-contracts-p3400 -fcontracts-p3850 %libcxx_flags -fsyntax-only -Xclang -verify %s
// REQUIRES: contracts-libcxx

// DECISIONS.md E175: -fcontracts-p3850 with an explicit -fno-contracts-p3400
// still implies P4451, whatever the order: the macros are raised, and
// <contracts> declares P4451's labels::terminating alone.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-additive-ftm-p3850-no-p3400.C

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
#ifdef __cpp_lib_contracts_labels
#error "__cpp_lib_contracts_labels"
#endif

constexpr auto &t = std::contracts::labels::terminating;
auto e = std::contracts::labels::empty_label; // expected-error {{no member named 'empty_label'}}
void f(int x) pre<terminating>(x > 0);
