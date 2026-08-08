// A handle_contract_violation whose parameter points to a non-const
// contract_violation_t is diagnosed (D4299).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-nonconst.c.)

#include <contracts.h>

void handle_contract_violation(contract_violation_t *cv) { // expected-error {{must have signature}}
  (void)cv;
}
