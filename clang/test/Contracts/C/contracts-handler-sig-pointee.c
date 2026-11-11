// A handle_contract_violation taking a pointer to something other than
// contract_violation_t is diagnosed (P4299).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s -o %t.o
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-pointee.c.)

#include <contracts.h>

void handle_contract_violation(const int *cv) { // expected-error {{'handle_contract_violation' must have signature 'void(const contract_violation_t *)' to be used as a contract-violation handler}}
  (void)cv;
}
