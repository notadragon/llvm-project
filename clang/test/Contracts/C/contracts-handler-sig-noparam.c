// A handle_contract_violation with no parameter is diagnosed (P4299).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-noparam.c.)

#include <contracts.h>

void handle_contract_violation(void) { // expected-error {{'handle_contract_violation' must have signature 'void(const contract_violation_t *)' to be used as a contract-violation handler}}
}
