// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/i.o %t/e.o %t/main.o -o %t/ie && %t/ie
//
// What an enforce check in a function another translation unit may provide
// implies must not reach the callers compiled with it: a definition linked from
// a TU that ignores the check would turn a program without undefined behavior
// into a wrong one.  In e.cpp the inline function f enforces x > 0, and its
// violation entry point is noreturn, but f's definition is linkonce_odr, which
// LLVM does not treat as exact: use_f keeps its test of `f (y) > 0'. i.cpp
// defines the same f with the check ignored, and the linker keeps that copy
// (the first it sees): use_f (-5) is 0. Clang passes; so does gnu_gcc since
// GCC-894 was fixed there by marking f a refinable body.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-enforce-replaceable.C
// (gnu_gcc's test for GCC-894, fixed)

//--- e.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // enforce
  return x;
}

int use_f (int y) { return f (y) > 0; }

//--- i.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  return x;
}

int (*use_fp) (int) = f;

//--- main.cpp
#include <cstdlib>

int use_f (int);

int main ()
{
  if (use_f (-5) != 0)
    std::abort ();
}
