// RUN: %clang -fcontracts-p4299 -fcontracts-p4405 -fcontract-evaluation-semantic=observe -fcontract-analysis-semantic=diagnose -Wno-contract-unproven %libcxx_flags %s -o %t -Xclang -verify && %t
// REQUIRES: contracts-libcxx, native

// D4405 in C: the analysis evaluation never evaluates the predicate, and
// the runtime evaluations keep their semantic (here observe): each
// assertion is diagnosed (silenced here) and still checked once at run
// time.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-analysis-run.c

// expected-no-diagnostics

#include <contracts.h>

static int evaluations, violations;

__attribute__((noinline)) static int positive(int x) {
  ++evaluations;
  __asm__ volatile("" ::: "memory");
  return x > 0;
}

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  ++violations;
}

int guarded(int x) _Pre(positive(x)) _Post(r: positive(r)) {
  _ContractAssert(positive(x));
  return x;
}

int main(void) {
  guarded(-1);
  if (evaluations != 3 || violations != 3)
    __builtin_abort();
  return 0;
}
