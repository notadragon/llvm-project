// RUN: %clang -fcontracts-p4299 %libcxx_flags %s -o %t && %t

// Each return site checks the _Post predicate afresh, so a GNU "?:" in it is
// evaluated on every return path.
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-post-shared-save-expr.c

#include <contracts.h>

static int calls = 0;
static int count(void) { return ++calls; }

int f(int x) _Post(r: __extension__(count() ?: 1) > 0) {
  if (x > 10)
    return 1;
  return x;
}

int main(void) { return f(3) == 3 && calls == 1 ? 0 : 1; }

// REQUIRES: contracts-libcxx, native
