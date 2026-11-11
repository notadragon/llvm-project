// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=observe %libcxx_flags %s -o %t && %t

// In a void function, "return g ();" runs g () before the postcondition is
// checked.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-return-void-expr.c

#include <contracts.h>

static int handled = 0;
void handle_contract_violation(const contract_violation_t *cv) { ++handled; }

static int v;
static void g(void) { v = 1; }

void f(void) _Post(v == 1) { return g(); }

int main(void) {
  f();
  return v == 1 && handled == 0 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
