// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s -o %t.o

// The replacement handler must be void(const contract_violation_t *)
// (P4299); a "const volatile contract_violation_t *" parameter is rejected.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-violation-handler-cv-param.c

#include <contracts.h>

void handle_contract_violation(const volatile contract_violation_t *cv) { // expected-error {{must have signature}}
  (void)cv;
}
