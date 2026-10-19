// RUN: %clang -fcontracts-p4299 -fcontract-configuration-file=%S/c-dynamic-selection-adjust.json %libcxx_flags %s -o %t && %t

// A dynamic selector may return any evaluation semantic, and one C does not
// allow is adjusted as D3595R1 ("Adjusting a Semantic to the Allowed Set")
// describes rather than being an invalid selection: without
// -fcontracts-allow-assume, assume is adjusted to ignore (the predicate is
// not evaluated), and without -fcontracts-p4298 noexcept_observe and
// noexcept_enforce are reported as observe and enforce.  The C++ form is
// ../p3595-dynamic-adjust-selector.cpp.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-dynamic-selection-adjust.c

#include <contracts.h>
#include <stdlib.h>

static unsigned short chosen;
unsigned short c_dyn_adjust_sel(void) { return chosen; }

static int evaluated, observed;
void handle_contract_violation(const contract_violation_t *cv) {
  int s = stdc_contract_violation_semantic(cv);
  if (s == STDC_CONTRACT_OBSERVE)
    ++observed;
  else if (s == STDC_CONTRACT_ENFORCE &&
           chosen == STDC_CONTRACT_NOEXCEPT_ENFORCE)
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
