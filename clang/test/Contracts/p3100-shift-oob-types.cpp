// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -Wno-return-type -Wno-shift-count-overflow -Wno-shift-count-negative -fcontract-configuration-file=%S/p3100-shift-oob-ignore.json %libcxx_flags -o %t && %t

// P3100: the shift out-of-range predicate uses the promoted left operand's
// width, so the same amount is UB for a narrower type but valid for a wider one.

#include <contracts>
#include <cstdlib>

namespace {
  int si (int a, int b) { return a << b; }
  unsigned su (unsigned a, int b) { return a << b; }
  long sl (long a, int b) { return a << b; }
  long long sll (long long a, int b) { return a << b; }
}

int main () {
  if (si (1, 40) != 0) std::abort ();              // 40 >= 32 -> UB -> 0
  if (sl (1L, 40) != (1L << 40)) std::abort ();    // 40 < 64 -> valid
  if (sll (1LL, 40) != (1LL << 40)) std::abort ();

  if (su (1u, 100) != 0) std::abort ();
  if (su (1u, 5) != 32u) std::abort ();

  if (si (1, 5) != 32) std::abort ();
  if (sl (3L, 2) != 12L) std::abort ();
}
