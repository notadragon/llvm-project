// RUN: %clang -std=c++26 -fno-contracts-p3850 -fcontracts-allow-assume -fno-contracts -fsyntax-only -Xclang -verify %s 2>&1 | FileCheck %s
// RUN: %clang -std=c++23 -fno-contracts-p3400 -fno-contracts -fsyntax-only -Xclang -verify %s 2>&1 | FileCheck --allow-empty --check-prefix=QUIET %s
// expected-no-diagnostics

// With -fno-contracts, no per-paper contracts feature-test macro (nor vendor
// macro) is defined, as __cpp_contracts is not, and -fcontracts-allow-assume
// has no effect.  A per-paper option given positively with -fno-contracts is
// an error (contract-options-rule-no-contracts-*.cpp, DECISIONS.md E176).
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
#if defined(__clang_contracts_p3100) || defined(__clang_contracts_p3290) \
    || defined(__clang_contracts_p4451) || defined(__clang_contracts_allow_assume)
#error "a Clang contracts vendor macro is defined with contracts off"
#endif

// CHECK: warning: '-fcontracts-allow-assume' has no effect without '-fcontracts'
// CHECK-NOT: warning:
// QUIET-NOT: warning:
