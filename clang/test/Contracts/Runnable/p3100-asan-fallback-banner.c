// RUN: %clang -std=c23 -g -fsanitize=address -fsanitize-recover=address \
// RUN:   -fcontracts-p3100 -fcontracts-p4298 -c %s -o %t.o
// RUN: %clang -fsanitize=address %t.o -o %t && not %t 2>&1 | FileCheck %s

// A C object compiled with -fcontracts-p3100 routes the address check (the
// descriptor is emitted); linked without the contracts runtime -- this link
// names no contracts option, and the driver links the runtime when one does
// (p3100-c-link-driver.c) -- the report falls back to the stock one.  That
// report must be whole: its first line, the "=====" banner, is printed only
// once the runtime knows no handler is linked.

#include <stdlib.h>

int main(void) {
  int *p = malloc(4 * sizeof(int));
  int v = p[4];
  free(p);
  return v;
}

// CHECK: {{^=+$}}
// CHECK-NEXT: ERROR: AddressSanitizer: heap-buffer-overflow
