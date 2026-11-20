// RUN: %clang -fcontracts-p4299 -fcontracts-p4405 -fcontract-configuration-file=%S/c-p4405-dynamic.json %libcxx_flags %s -o %t -Xclang -verify && %t
// RUN: %clang -fcontracts-p4299 -fcontract-configuration-file=%S/c-p4405-dynamic.json %libcxx_flags %s -o %t.noflag -Xclang -verify && %t.noflag
// REQUIRES: contracts-libcxx, native
// expected-no-diagnostics

// P4405 in C: a dynamic selector returning prove (8) or diagnose (9) takes
// the ignore path -- the predicate is not evaluated, nothing is reported,
// and nothing is diagnosed at compile time -- with -fcontracts-p4405 and,
// adjusted to the allowed set, without it.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-dynamic.c and
// c-p4405-dynamic-no-flag.c (the second RUN line)

#include <contracts.h>
#include <stdlib.h>

static unsigned short chosen;
unsigned short c_p4405_sel(void) { return chosen; }

static int evaluated, observed;
void handle_contract_violation(const contract_violation_t *cv) {
  if (stdc_contract_violation_semantic(cv) == STDC_CONTRACT_OBSERVE)
    ++observed;
  else
    abort();
}

static int chk(int x) { ++evaluated; return x > 0; }
int f(int x) _Pre(chk(x)) { return x; }

int main(void) {
  chosen = 8; /* prove */
  f(-1);
  chosen = 9; /* diagnose */
  f(-1);
  if (evaluated != 0 || observed != 0)
    abort();
  chosen = STDC_CONTRACT_OBSERVE;
  f(-1);
  if (evaluated != 1 || observed != 1)
    abort();
  return 0;
}
