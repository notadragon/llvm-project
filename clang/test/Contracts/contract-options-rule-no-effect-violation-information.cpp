// RUN: %clang -std=c++26 -fno-contracts -fcontract-violation-information=none -fsyntax-only -Xclang -verify %s 2>&1 \
// RUN:   | FileCheck --allow-empty %s

// DECISIONS.md E175-E178: the contracts option rule.
// -fcontract-violation-information= does not warn with contracts off
// (E178 (1)): the libraries' violation records honour it whatever the
// contracts options.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/contract-options-rule-no-effect-violation-information.C

// CHECK-NOT: {{warning|error}}:

// expected-no-diagnostics

#ifndef __clang_contracts_violation_information_none
#error "__clang_contracts_violation_information_none"
#endif

int i;
