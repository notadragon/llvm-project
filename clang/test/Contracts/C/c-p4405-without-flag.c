// RUN: %clang -fcontracts-p4299 -fcontract-evaluation-semantic=prove %libcxx_flags %s -o %t 2> %t.err
// RUN: FileCheck %s < %t.err
// RUN: %t
// REQUIRES: contracts-libcxx, native

// P4405 in C: the prove semantic without -fcontracts-p4405 is not allowed;
// the compiler says so, and ignore is used instead, so the predicate is not
// evaluated and nothing is diagnosed.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-p4405-without-flag.c

#include <contracts.h>

static int evaluations;

static int positive(int x) {
  ++evaluations;
  return x > 0;
}

int guarded(int x) _Pre(positive(x)) { return x; }

int main(void) {
  guarded(-1);
  return evaluations;
}

// CHECK: warning: '-fcontract-evaluation-semantic=prove' requires '-fcontracts-p4405'; 'ignore' is used instead
// CHECK-NOT: cannot be proven
