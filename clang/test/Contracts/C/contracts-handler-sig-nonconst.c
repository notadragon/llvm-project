// A handle_contract_violation whose parameter points to a non-const
// contract_violation_t is diagnosed (P4299).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-nonconst.c.)

#include <contracts.h>

void handle_contract_violation(contract_violation_t *cv) { // expected-error {{'handle_contract_violation' must have signature 'void(const contract_violation_t *)' to be used as a contract-violation handler}}
  (void)cv;
}
