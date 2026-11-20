// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-overflow-folded-ignore.json %libcxx_flags -o %t && %t
// RUN: %clangxx -O2 -std=c++26 %s -fcontracts -fcontracts-p3100 -fcontract-configuration-file=%S/p3100-overflow-folded-ignore.json %libcxx_flags -o %t2 && %t2

// Under `ignore' a P3100 signed overflow has the defined wrapped value, also in
// the shapes the folders would simplify by assuming it cannot happen:
// `x + 1 > x' is false and `x * 4 / 4' is -1 for INT_MAX.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p3100-overflow-folded-ignore.C

#include <climits>

__attribute__((noinline)) bool gt (int x) { return x + 1 > x; }
__attribute__((noinline)) int muldiv (int x) { return x * 4 / 4; }

int main ()
{
  if (gt (INT_MAX))
    __builtin_abort ();
  if (muldiv (INT_MAX) != -1)
    __builtin_abort ();
}
