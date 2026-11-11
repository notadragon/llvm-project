// RUN: %clang -fcontracts-p4299 -fcontract-configuration-file=%S/c-dynamic-selection-observe.json %libcxx_flags %s -o %t && %t

// A dynamic selector overrides its entry's static semantic, for a
// precondition, a postcondition and a contract assertion alike: each is
// checked under the observe the selector returns, not the static enforce,
// so each violation reaches the handler and the program continues.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-dynamic-selection-observe.c

#include <contracts.h>

static int calls = 0, handled = 0;
int c_dyn_obs_sel (void) { ++calls; return STDC_CONTRACT_OBSERVE; }
void handle_contract_violation (const contract_violation_t *cv) { ++handled; }

int f (int x) _Pre (x > 0) _Post (r : r > 10) { _ContractAssert (x > 5); return x; }

int main (void)
{
  f (-1);
  return calls == 3 && handled == 3 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
