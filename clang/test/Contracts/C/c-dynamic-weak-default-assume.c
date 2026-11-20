// RUN: %clang -fcontracts-p4299 -fcontracts-allow-assume -fcontract-configuration-file=%S/../p3595-dynamic-weak-default-assume.json %libcxx_flags %s -o %t && %t

// With -fcontracts-allow-assume, a dynamically selected contract whose
// configuration names assume as the default semantic gets a weak default
// selector that returns assume, which takes the ignore path: the predicate
// is not evaluated and nothing is reported (CLANG-673 was an enforced
// violation here).  The C++ form is ../p3595-dynamic-weak-default-assume.cpp,
// whose configuration file this test shares.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-dynamic-weak-default-assume.c

#include <contracts.h>

static int violations, evaluated;

void handle_contract_violation(const contract_violation_t *cv) {
  (void)cv;
  ++violations;
}

static int chk(int x) {
  ++evaluated;
  return x > 0;
}

int f(int x) _Pre(chk(x)) { return x; }

int main(void) {
  f(-1);
  return violations + evaluated;
}

// REQUIRES: contracts-libcxx, native
