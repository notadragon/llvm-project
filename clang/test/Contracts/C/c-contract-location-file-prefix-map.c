// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe -ffile-prefix-map==FILE-PREFIX %libcxx_flags %s -o %t && %t
// REQUIRES: contracts-libcxx, native

// -ffile-prefix-map remaps the file name a C contract violation reports, as
// it does __FILE__: for a precondition, a postcondition and a contract
// assertion.
//
// Mirror, in the gnu_gcc fork:
// gcc/testsuite/gcc.dg/contracts/c-contract-location-file-prefix-map.c.

#include <contracts.h>
#include <string.h>

static int count;

void handle_contract_violation(const contract_violation_t *cv) {
  const char *file = stdc_contract_violation_file(cv);
  if (strncmp(file, "FILE-PREFIX", 11) != 0 || strcmp(file, __FILE__) != 0)
    __builtin_abort();
  ++count;
}

int pre_f(int x) _Pre(x > 0) { return x; }
int post_f(int x) _Post(r : r > 0) { return x; }
void assert_f(int x) { _ContractAssert(x > 0); }

int main(void) {
  pre_f(0);
  post_f(0);
  assert_f(0);
  if (count != 3)
    __builtin_abort();
  return 0;
}
