// RUN: %clang -std=c++26 -fcontracts-p3850 -fcontracts-allow-assume -fno-contracts -Wno-unused-command-line-argument -fsyntax-only -Xclang -verify %s
// RUN: %clang -std=c++23 -fcontracts-p3400 -fno-contracts -Wno-unused-command-line-argument -fsyntax-only -Xclang -verify %s
// expected-no-diagnostics

// With -fno-contracts, no per-paper contracts feature-test macro (nor vendor
// macro) is defined, as __cpp_contracts is not.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/ftm-without-contracts.C

#if defined(__cpp_contracts)
#error "__cpp_contracts defined with contracts off"
#endif
#if defined(__cpp_contracts_labels) || defined(__cpp_contracts_message) \
    || defined(__cpp_contracts_requires) || defined(__cpp_contracts_report) \
    || defined(__cpp_contracts_postcondition_captures) \
    || defined(__cpp_contracts_nonthrowing_semantics)
#error "a per-paper contracts feature-test macro is defined with contracts off"
#endif
#if defined(__gcc_contracts_p3100) || defined(__gcc_contracts_p3290) \
    || defined(__gcc_contracts_allow_assume)
#error "a GCC contracts vendor macro is defined with contracts off"
#endif
