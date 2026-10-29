// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clang -std=c23 -fcontracts-p4299 -O2 -fno-inline %libcxx_flags -fcontract-evaluation-semantic=observe -c %t/e.c -o %t/e.o
// RUN: %clang -std=c23 -fcontracts-p4299 -O2 -fno-inline %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.c -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824, the memory form for a C99 inline function: i.c's inline
// definition of f (assertion ignored; available_externally, not exact)
// stores nothing, but LLVM infers nothing from it, so use_g reloads g after
// the call.  -fno-inline keeps the call out of line, and the C99 rule sends
// it to the external definition in e.c (assertion observed), whose
// violation handler sets g to 5 and returns: use_g (-5) is 0.  Correct
// before the fix too.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-c99-pure.C
// (gnu_gcc's GCC-896)

//--- e.c
#include <contracts.h>

inline int f (int x)
{
  _ContractAssert (x > 0);  /* observe */
  return x;
}

extern int f (int);

//--- i.c
#include <contracts.h>

int g;

inline int f (int x)
{
  _ContractAssert (x > 0);  /* ignore */
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

extern "C" int g;
int handled;

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  g = 5;
}

extern "C" int use_g (int);

int main ()
{
  int r = use_g (-5);
  if (handled != 1 || r != 0)
    std::abort ();
}
