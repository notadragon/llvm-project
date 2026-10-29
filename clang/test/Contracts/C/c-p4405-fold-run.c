// RUN: %clang -fcontracts-p4299 -fcontracts-p4405 -fcontracts-prove-assume -fcontract-evaluation-semantic=diagnose %libcxx_flags %s -o %t -Xclang -verify && %t
// REQUIRES: contracts-libcxx, native

// P4405 stage 2 in C: folding never evaluates the predicate at run time --
// a predicate true whatever its operand's side effects is not proven, and
// none of them run -- and -fcontracts-prove-assume is accepted and changes
// nothing.  diagnose is used so that the program links and runs.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-fold-run.c (GCC folds
// x == x, Clang does not; GCC's proof shadow, switched off there, does not
// exist in Clang)

#include <contracts.h>

static int evaluations;

static int touch(void) {
  ++evaluations;
  return 0;
}

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  __builtin_abort();
}

int guarded(int x)
  _Pre(touch() || 1)            // expected-warning {{contract assertion cannot be proven}}
  _Pre(x == x)                  // expected-warning {{contract assertion cannot be proven}} \
                                // expected-warning {{self-comparison always evaluates to true}}
  _Post(r: (touch(), 1))        // expected-warning {{contract assertion cannot be proven}}
{
  _ContractAssert(touch() && 0); // expected-warning {{contract assertion cannot be proven}}
  _ContractAssert(0);           // expected-warning {{contract predicate is always false}}
  _ContractAssert(1);
  return x;
}

int main(void) {
  guarded(-1);
  if (evaluations != 0)
    __builtin_abort();
  return 0;
}
