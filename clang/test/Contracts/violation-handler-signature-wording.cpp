// RUN: %clangxx -std=c++26 -fcontracts %libcxx_flags -fsyntax-only -Xclang -verify %s

// The diagnostics for a declaration of ::handle_contract_violation that cannot
// replace the violation handler, worded with "must" like Clang's other
// signature diagnostics ("'main' must return 'int'").  Overloads, so that
// each declaration is checked on its own.
//
// Mirror of gnu_gcc's violation-handler-signature-wording.C.

#include <contracts>

using std::contracts::contract_violation;

inline void handle_contract_violation(const contract_violation &) {} // expected-error {{'::handle_contract_violation' must not be declared 'inline'}}
void handle_contract_violation(); // expected-error {{'::handle_contract_violation' must have a single parameter of type 'const std::contracts::contract_violation&'}}
void handle_contract_violation(const contract_violation &, int); // expected-error {{'::handle_contract_violation' must have a single parameter of type 'const std::contracts::contract_violation&'}}
void handle_contract_violation(contract_violation &&); // expected-error {{parameter of '::handle_contract_violation' must be an lvalue reference to 'const std::contracts::contract_violation'}}
void handle_contract_violation(contract_violation &); // expected-error {{parameter of '::handle_contract_violation' must be a reference to 'const std::contracts::contract_violation'}}
void handle_contract_violation(const int &); // expected-error {{parameter of '::handle_contract_violation' must be of type 'const std::contracts::contract_violation&'}}
int handle_contract_violation(const contract_violation *); // expected-error {{'::handle_contract_violation' must return 'void'}} \
                                                           // expected-error {{parameter of '::handle_contract_violation' must be an lvalue reference to 'const std::contracts::contract_violation'}}
extern "C" void handle_contract_violation(const volatile contract_violation &); // expected-error {{'::handle_contract_violation' must have C++ language linkage}} \
                                                                                // expected-error {{parameter of '::handle_contract_violation' must be a reference to 'const std::contracts::contract_violation'}}

// REQUIRES: contracts-libcxx
