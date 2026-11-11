// DEFINE: %{cc} = %clangxx -std=c++26 %libcxx_flags -fsyntax-only -Xclang -verify %s
//
// RUN: %{cc} -fcontracts-p4451
// RUN: %{cc} -fcontracts-p3400 -DP3400
// RUN: %{cc} -fcontracts-p3400 -fcontracts-p4451 -DP3400
// RUN: %{cc} -fcontracts-p3850 -DP3400 -DP3097
// RUN: %{cc} -fcontracts -fcontracts-p4451
// REQUIRES: contracts-libcxx

// expected-no-diagnostics

// P4451: feature-test macros.  Under -fcontracts-p4451, and under
// -fcontracts-p3400 and -fcontracts-p3850, which imply it,
// __cpp_contracts and __cpp_lib_contracts are raised from P2900's 202502L
// (to D4451R0's 202611L, not pinned) and __clang_contracts_p4451 is
// defined; the P3400 library is there only with P3400.  (A later
// -fno-contracts-p4451: p4451-diag-no-flag.cpp.)
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4451-ftm.C, p4451-ftm-p3400.C
// and p4451-ftm-p3850.C

#include <contracts>  // `terminating`

#if !(__cpp_contracts > 202502L)
#error "__cpp_contracts"
#endif
#if !(__cpp_lib_contracts > 202502L)
#error "__cpp_lib_contracts"
#endif
#if !(__clang_contracts_p4451 > 0)
#error "__clang_contracts_p4451"
#endif
#ifdef P3097
// P3850 implies P3097 too, whose value tracks the month as
// __clang_contracts_p4451 does; __cpp_contracts is the greater of that and
// P4451's value, which __cpp_lib_contracts carries.
#if !(__cpp_contracts >= __cpp_lib_contracts)
#error "__cpp_contracts below P4451's value"
#endif
#if !(__cpp_contracts >= __clang_contracts_p4451)
#error "__cpp_contracts below P3097's value"
#endif
#else
// Without P3097, nothing else raises __cpp_contracts: both macros carry
// P4451's value.
#if __cpp_contracts != __cpp_lib_contracts
#error "__cpp_contracts and __cpp_lib_contracts differ"
#endif
#endif
#if defined(P3400) != defined(__cpp_lib_contracts_labels)
#error "__cpp_lib_contracts_labels"
#endif

constexpr auto &t = std::contracts::labels::terminating;
