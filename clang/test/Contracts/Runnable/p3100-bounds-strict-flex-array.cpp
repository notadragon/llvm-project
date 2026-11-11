// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fstrict-flex-arrays=3 -fcontract-configuration-file=%S/p3100-bounds-strict-flex-array.json %libcxx_flags -o %t && %t

// With -fstrict-flex-arrays=3 only a trailing [] array is a flexible array
// member, so the P3100 array-bounds check applies to a trailing [1] or GNU
// zero-length array like any other: an index past the bound is a violation.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-bounds-strict-flex-array.C

#include <contracts>
#include <cstdlib>

static int handled = 0;
void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
}

struct Z { int n; __extension__ int a[0]; };
struct One { int n; int a[1]; };

int main ()
{
  Z *z = (Z *) std::calloc (1, sizeof (Z) + 4 * sizeof (int));
  One *o = (One *) std::calloc (1, sizeof (One) + 4 * sizeof (int));
  volatile int two = 2;
  int r = z->a[two] + o->a[two];
  (void) r;
  std::free (z);
  std::free (o);
  return handled == 2 ? 0 : 1;
}
