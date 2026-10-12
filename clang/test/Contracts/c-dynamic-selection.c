// RUN: %clang -fcontracts-p4299 -fcontract-configuration-file=%S/c-dynamic-selection.json %libcxx_flags %s -o %t && %t

// A P3595 configuration entry with an output.dynamic selector and no static
// semantic: C calls the selector at the check, and its choice decides.  Here
// it answers ignore, so it is called once and the handler never.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-dynamic-selection.c

#include <contracts.h>

static int calls = 0, handled = 0;
int c_dyn_sel(void) { ++calls; return STDC_CONTRACT_IGNORE; }
void handle_contract_violation(const contract_violation_t *cv) { ++handled; }

int f(int x) _Pre(x > 0) { return x; }

int main(void) {
  f(-1);
  return calls == 1 && handled == 0 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
