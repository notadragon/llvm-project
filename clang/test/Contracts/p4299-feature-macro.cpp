// RUN: %clang_cc1 -std=c++26 -fcontracts -fcontracts-p4299 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fcontracts -fsyntax-only -verify -DWITHOUT %s
// expected-no-diagnostics

// -fcontracts-p4299 in C++ predefines __clang_contracts_p4299 too, since
// _Pre, _Post and _ContractAssert are keywords there as well; without it the
// macro is not defined.  Its value is the date of the implemented revision,
// so only check that it is positive.
//
// Mirror of gnu_gcc's p4299-feature-macro.C and p4299-feature-macro-without.C.

#ifdef WITHOUT
#ifdef __clang_contracts_p4299
#error "__clang_contracts_p4299 without -fcontracts-p4299"
#endif
#elif !defined(__clang_contracts_p4299) || __clang_contracts_p4299 <= 0
#error "__clang_contracts_p4299"
#endif
