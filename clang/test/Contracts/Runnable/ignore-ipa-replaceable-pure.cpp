// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=observe -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824, the memory form: i.cpp's copy of the inline f (assertion
// ignored) stores nothing, but its linkonce_odr definition is not exact, so
// LLVM does not infer that it leaves memory alone, and use_g reloads g after
// the call.  The linker keeps e.cpp's copy (assertion observed), whose
// violation handler sets g to 5 and returns: use_g (-5) is -5 + 5 = 0.
// Correct before the fix too (GCC was not: it folded the reload to 1).
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-pure.C
// (gnu_gcc's GCC-896)

//--- e.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // observe
  return x;
}

int (*use_fp_e) (int) = f;

//--- i.cpp
int g;

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}

int use_g (int y)
{
  g = 1;
  int r = f (y);
  return r + g;
}

//--- main.cpp
#include <contracts>
#include <cstdlib>

extern int g;
int handled;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  g = 5;
}

int use_g (int);

int main ()
{
  int r = use_g (-5);
  if (handled != 1 || r != 0)
    std::abort ();
}
