// A handle_contract_violation with too many parameters is diagnosed (D4299).
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-twoparam.c.)

#include <contracts.h>

void handle_contract_violation(const contract_violation_t *cv, int extra) { // expected-error {{must have signature}}
  (void)cv;
  (void)extra;
}
