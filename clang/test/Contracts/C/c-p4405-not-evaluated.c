// RUN: %clang -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=diagnose -Wno-contract-unproven %libcxx_flags %s -o %t && %t
// REQUIRES: contracts-libcxx, native

// P4405 in C: prove and diagnose have no run-time effect -- the predicate is
// never evaluated and the handler never called.  diagnose is used, with
// -Wno-contract-unproven, so that the program links and runs silently.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-not-evaluated.c

#include <contracts.h>

static int evaluations;

static int positive(int x) {
  ++evaluations;
  return x > 0;
}

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  __builtin_abort();
}

int guarded(int x) _Pre(positive(x)) _Post(r: positive(r)) {
  _ContractAssert(positive(x));
  return x;
}

int main(void) {
  guarded(-1);
  if (evaluations != 0)
    __builtin_abort();
  return 0;
}
