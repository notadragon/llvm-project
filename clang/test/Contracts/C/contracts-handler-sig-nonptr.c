// A handle_contract_violation whose parameter is not a pointer is diagnosed.
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-nonptr.c.)

#include <contracts.h>

void handle_contract_violation(int cv) { // expected-error {{'handle_contract_violation' must have signature 'void(const contract_violation_t *)' to be used as a contract-violation handler}}
  (void)cv;
}
