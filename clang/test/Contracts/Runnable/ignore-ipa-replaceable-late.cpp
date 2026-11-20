// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824: as ignore-ipa-replaceable-dtor.cpp, with f also calling an
// inline h defined after it, so that in GCC the "cannot throw" fact comes
// from the middle end rather than the front end.  In Clang f's call of h
// (no nounwind when f is emitted, and h's linkonce_odr definition gives
// LLVM nothing to infer from) kept f from being marked even before the fix;
// the test guards that it stays so.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-late.C
// (gnu_gcc's GCC-896)

//--- e.cpp
inline void h ();

[[gnu::noinline]] inline int f (int x)
{
  h ();
  contract_assert (x > 0);  // enforce
  return x;
}

inline void h () {}

int (*use_fp_e) (int) = f;

//--- i.cpp
int dtors;
struct D { ~D () { ++dtors; } };

inline void h ();

[[gnu::noinline]] inline int f (int x)
{
  h ();
  contract_assert (x > 0);  // ignore
  return x;
}

int use_dtor (int y)
{
  D d;
  return f (y);
}

inline void h () {}

//--- main.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

int use_dtor (int);
extern int dtors;

int main ()
{
  try
    {
      use_dtor (-5);
    }
  catch (int)
    {
      if (dtors != 1)
        std::abort ();
      return 0;
    }
  std::abort ();
}
