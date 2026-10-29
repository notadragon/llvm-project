// RUN: %clang -fcontracts-p4299 -fcontracts-p4298 -fcontracts-allow-assume -fcontract-configuration-file=%S/c-dynamic-selection-adjust.json %libcxx_flags %s -o %t && %t

// With -fcontracts-p4298 a dynamic selector's noexcept semantics are used as
// they are, and with -fcontracts-allow-assume an assume takes the ignore
// path: the handler sees STDC_CONTRACT_NOEXCEPT_OBSERVE and
// STDC_CONTRACT_NOEXCEPT_ENFORCE.  c-dynamic-selection-adjust.c is the form
// without the flags.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-dynamic-selection-noexcept.c

#include <contracts.h>
#include <stdlib.h>

static unsigned short chosen;
unsigned short c_dyn_adjust_sel(void) { return chosen; }

static int evaluated, observed;
void handle_contract_violation(const contract_violation_t *cv) {
  int s = stdc_contract_violation_semantic(cv);
  if (s == STDC_CONTRACT_NOEXCEPT_OBSERVE)
    ++observed;
  else if (s == STDC_CONTRACT_NOEXCEPT_ENFORCE)
    _Exit(0);
  else
    abort();
}

static int chk(int x) {
  ++evaluated;
  return x > 0;
}
int f(int x) _Pre(chk(x)) { return x; }

int main(void) {
  chosen = STDC_CONTRACT_ASSUME;
  f(-1);
  if (evaluated != 0 || observed != 0)
    abort();
  chosen = STDC_CONTRACT_NOEXCEPT_OBSERVE;
  f(-1);
  if (evaluated != 1 || observed != 1)
    abort();
  chosen = STDC_CONTRACT_NOEXCEPT_ENFORCE;
  f(-1);
  abort();
}

// REQUIRES: contracts-libcxx, native
