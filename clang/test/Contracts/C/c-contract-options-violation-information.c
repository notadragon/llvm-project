// RUN: %clang -std=c23 -fcontract-violation-information=none -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E178: the contracts option rule in C, where
// -fcontract-violation-information= does not warn without -fcontracts-p4299:
// <contracts.h> honours it whatever the contracts options.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-contract-options-violation-information.c

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifndef __clang_contracts_violation_information_none
#error "__clang_contracts_violation_information_none"
#endif

int i;
