// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824 (fixed): as ignore-ipa-replaceable-catch.cpp, for a local's
// destructor.  i.cpp's use_dtor has a local with a destructor and calls the
// inline f, whose assertion that TU ignores; f was marked nounwind there, so
// use_dtor had no cleanup for the call and the destructor did not run when
// the linked copy of f (e.cpp's, assertion enforced) called the throwing
// violation handler.  main catches the exception; the destructor must have
// run once.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-dtor.C
// (gnu_gcc's GCC-896)

//--- e.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}

int (*use_fp_e) (int) = f;

//--- i.cpp
int dtors;
struct D { ~D () { ++dtors; } };

[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}

int use_dtor (int y)
{
  D d;
  return f (y);
}

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
