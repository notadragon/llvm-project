// RUN: %clang -fcontracts-p4299 -fcontract-configuration-file=%S/c-dynamic-selection-weak.json %libcxx_flags %s -o %t && %t

// A dynamic selector with provideweak that the program does not define: the
// weak default, which returns the entry's configured semantic (observe),
// decides.
// The C++-linkage name is mangled as C++ would mangle it (the program
// links only if the weak default carries that name).
//
// Mirror: gcc/testsuite/gcc.dg/contracts/c-dynamic-selection-weak.c

#include <contracts.h>

static int handled = 0;
void handle_contract_violation (const contract_violation_t *cv) { ++handled; }

int f (int x) _Pre (x > 0) { return x; }

int main (void)
{
  f (-1);
  return handled == 1 ? 0 : 1;
}

// REQUIRES: contracts-libcxx, native
