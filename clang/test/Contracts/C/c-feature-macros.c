// RUN: %clang_cc1 -fcontracts-p4299 -fsyntax-only -verify %s
// RUN: %clang_cc1 -fsyntax-only -verify -DWITHOUT %s
// expected-no-diagnostics

// With -fcontracts-p4299 the compiler predefines __clang_contracts_p4299
// (P4299's keywords need no header), and <contracts.h> defines
// __STDC_VERSION_CONTRACTS_H__, the C23 convention for a header's version.
// Both carry the date of the implemented revision until P4299 is adopted, so
// only check that they are positive.  Without -fcontracts-p4299 the compiler
// macro is not defined.
//
// Mirror of gnu_gcc's gcc.dg/contracts/c-feature-macros.c and
// c-feature-macros-without-p4299.c.

#ifdef WITHOUT
#ifdef __clang_contracts_p4299
#error "__clang_contracts_p4299 without -fcontracts-p4299"
#endif
#else
#if !defined(__clang_contracts_p4299) || __clang_contracts_p4299 <= 0
#error "__clang_contracts_p4299"
#endif

#ifdef __STDC_VERSION_CONTRACTS_H__
#error "__STDC_VERSION_CONTRACTS_H__ before <contracts.h>"
#endif

#include <contracts.h>

#if !defined(__STDC_VERSION_CONTRACTS_H__) || __STDC_VERSION_CONTRACTS_H__ <= 0
#error "__STDC_VERSION_CONTRACTS_H__"
#endif
#endif
