// A handle_contract_violation with no parameter is diagnosed (D4299).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-noparam.c.)

#include <contracts.h>

void handle_contract_violation(void) { // expected-error {{must have signature}}
}
