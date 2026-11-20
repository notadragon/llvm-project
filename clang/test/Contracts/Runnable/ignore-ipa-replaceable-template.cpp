// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824 (fixed): as ignore-ipa-replaceable-catch.cpp, for an implicit
// instantiation of a function template (linkonce_odr too).  use_catch (-5)
// must return -1; it called std::terminate.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-template.C
// (gnu_gcc's GCC-896)

//--- e.cpp
template <typename T>
[[gnu::noinline]] T f (T x)
{
  contract_assert (x > 0);  // enforce
  return x;
}

int (*use_fp_e) (int) = f<int>;

//--- i.cpp
template <typename T>
[[gnu::noinline]] T f (T x)
{
  contract_assert (x > 0);  // ignore
  return x;
}

int use_catch (int y)
{
  try
    {
      return f (y);
    }
  catch (...)
    {
      return -1;
    }
}

//--- main.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

int use_catch (int);

int main ()
{
  if (use_catch (-5) != -1)
    std::abort ();
}
