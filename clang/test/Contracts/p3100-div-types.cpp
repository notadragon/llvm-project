// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -fcontract-configuration-file=%S/p3100-div-by-zero-ignore.json %libcxx_flags -o %t && %t

// P3100: divide-by-zero guard across integer types and both / and % (distinct
// signed/unsigned and widened paths).  ignore: zero divisor -> defined 0.

#include <contracts>
#include <cstdlib>

namespace {
  unsigned du (unsigned a, unsigned b) { return a / b; }
  unsigned mu (unsigned a, unsigned b) { return a % b; }
  long dl (long a, long b) { return a / b; }
  long ml (long a, long b) { return a % b; }
  long long dll (long long a, long long b) { return a / b; }
  short ds (short a, short b) { return (short) (a / b); }
  char dc (char a, char b) { return (char) (a / b); }
  long dmix (long a, int b) { return a / b; }
}

int main () {
  if (du (10u, 0u) != 0) std::abort ();
  if (mu (10u, 0u) != 0) std::abort ();
  if (dl (10L, 0L) != 0) std::abort ();
  if (ml (10L, 0L) != 0) std::abort ();
  if (dll (10LL, 0LL) != 0) std::abort ();
  if (ds ((short) 10, (short) 0) != 0) std::abort ();
  if (dc ((char) 10, (char) 0) != 0) std::abort ();
  if (dmix (10L, 0) != 0) std::abort ();

  if (du (10u, 3u) != 3) std::abort ();
  if (mu (10u, 3u) != 1) std::abort ();
  if (dl (-10L, 3L) != -3) std::abort ();
  if (ml (-10L, 3L) != -1) std::abort ();
  if (dmix (100L, 7) != 14) std::abort ();
}
