// A handle_contract_violation whose parameter is not a pointer is diagnosed.
// RUN: %clang_cc1 -fcontracts-p4299 -emit-obj -verify %s
// (GCC mirror: gcc.dg/contracts/contracts-handler-sig-nonptr.c.)

#include <contracts.h>

void handle_contract_violation(int cv) { // expected-error {{must have signature}}
  (void)cv;
}
