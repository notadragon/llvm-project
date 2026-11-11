// Test that handle_contract_violation with wrong signature is diagnosed.
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-return.c.)

#include <contracts.h>

int handle_contract_violation(const contract_violation_t *cv) { // expected-error {{'handle_contract_violation' must have signature 'void(const contract_violation_t *)' to be used as a contract-violation handler}}
  return 0;
}
