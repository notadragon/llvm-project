// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify %s
// RUN: %clang_cc1 -std=c++26 -fno-contracts -fsyntax-only -verify %s

// With contracts off, the extension spelling __contract_assert is diagnosed
// ("requires '-fcontracts'"), not built into a contract.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/extension-spelling-without-contracts.C

void g(int x) { __contract_assert(x > 0); } // expected-error {{'__contract_assert' requires '-fcontracts'}}

// Recovery: the rest of the file is still parsed.
int after = "x"; // expected-error {{cannot initialize a variable of type 'int' with an lvalue of type 'const char[2]'}}
