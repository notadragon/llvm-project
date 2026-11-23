// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824 (fixed): "cannot throw" must not be derived from the body of a
// function another translation unit may provide, when that body holds a
// contract evaluation whose semantic another TU may choose differently.
// i.cpp defines the inline f with its assertion ignored; f's body then has
// no call, and TryMarkNoThrow used to mark the linkonce_odr f nounwind, so
// use_catch, compiled there, called it with `call' and lost its handler.
// e.cpp defines the same f with the assertion enforced, and the linker keeps
// that copy (the first it sees).  The violation handler throws, so
// use_catch (-5) must catch the exception and return -1; it called
// std::terminate.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-catch.C
// (gnu_gcc's GCC-896)

//--- e.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}

int (*use_fp_e) (int) = f;

//--- i.cpp
[[gnu::noinline]] inline int f (int x)
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
