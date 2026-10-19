// With -fcontracts-allow-assume, a C contract assertion whose semantic is
// assume behaves exactly like ignore (notadragon_wg21 DECISIONS.md A14): the
// predicate is not evaluated and the violation handler is not called.  Here
// the semantic comes from the command line; c-allow-assume-config.c selects
// it from a configuration file.
// RUN: %clang -fcontracts-p4299 -fcontracts-allow-assume -fcontract-evaluation-semantic=assume %libcxx_flags %s -o %t && %t
// (GCC mirror: gcc.dg/contracts/c-allow-assume-semantic.c.)

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

// REQUIRES: contracts-libcxx, native
