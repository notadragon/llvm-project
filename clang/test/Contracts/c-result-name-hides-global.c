// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

// The _Post result name is scoped to the postcondition predicate, so in the
// body a file-scope variable of the same name is visible, and f returns it.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-result-name-hides-global.c

#include <contracts.h>

static int handled = 0;
void handle_contract_violation(const contract_violation_t *cv) { ++handled; }

int r = 5;

int f(void) _Post(r: r > 0) { return r; }

int main(void) { return f() == 5 && handled == 0 ? 0 : 1; }

// REQUIRES: contracts-libcxx, native
