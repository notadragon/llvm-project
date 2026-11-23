// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-bounds-flex-array.json %libcxx_flags -o %t && %t

// The P3100 array-bounds check leaves the flexible-array idioms alone, as
// stock -fsanitize=bounds does: a GNU zero-length trailing array and, at the
// default -fstrict-flex-arrays, a trailing [1] array.  Under `ignore' the
// accesses past the declared bound reach the elements they name.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-bounds-flex-array.C

#include <cstdlib>
struct Z { int n; __extension__ int a[0]; };
struct One { int n; int a[1]; };
int main ()
{
  Z *z = (Z *) std::malloc (sizeof (Z) + 4 * sizeof (int));
  One *o = (One *) std::malloc (sizeof (One) + 4 * sizeof (int));
  for (int i = 0; i < 4; ++i)
    {
      z->a[i] = 10 * (i + 1);
      o->a[i] = 10 * (i + 1);
    }
  volatile int two = 2;
  if (z->a[two] != 30 || z->a[0] != 10)
    __builtin_abort ();
  if (o->a[two] != 30 || o->a[0] != 10)
    __builtin_abort ();
  std::free (z);
  std::free (o);
}
