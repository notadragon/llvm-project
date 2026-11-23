// A handle_contract_violation that is variadic is diagnosed (P4299): the
// replacement must be void(const contract_violation_t *).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s -o %t.o
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-variadic.c.)

#include <contracts.h>

void handle_contract_violation(const contract_violation_t *cv, ...) { // expected-error {{'handle_contract_violation' must have signature 'void(const contract_violation_t *)' to be used as a contract-violation handler}}
  (void)cv;
}
