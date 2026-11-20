// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clang -std=c23 -fcontracts-p4299 -fexceptions -O2 -fno-inline %libcxx_flags -c %t/e.c -o %t/e.o
// RUN: %clang -std=c23 -fcontracts-p4299 -fexceptions -O2 -fno-inline %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.c -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags -c %t/main.cpp -o %t/main.o
// RUN: %clangxx -std=c++26 -fcontracts -O2 %libcxx_flags %t/e.o %t/i.o %t/main.o -o %t/ei && %t/ei
//
// CLANG-824 (fixed): as ignore-ipa-replaceable-dtor.cpp, for a C99 inline
// function in C code compiled with -fexceptions, unwound by a C++ violation
// handler.  i.c has the inline definition of f with its assertion ignored
// (emitted available_externally when optimizing, which TryMarkNoThrow used
// to mark nounwind) and use_cleanup, whose local c has a cleanup.
// -fno-inline keeps the call out of line; the C99 rule sends it to the
// external definition in e.c (assertion enforced).  The violation handler
// throws: main catches the exception, and the cleanup must have run.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/refinable-body-ignore-replaceable-c99.C
// (gnu_gcc's GCC-896)

//--- e.c
#include <contracts.h>

inline int f (int x)
{
  _ContractAssert (x > 0);  /* enforce */
  return x;
}

extern int f (int);

//--- i.c
#include <contracts.h>

extern int cleanups;
static void cleanup_fn (int *p) { (void) p; ++cleanups; }

inline int f (int x)
{
  _ContractAssert (x > 0);  /* ignore */
  return x;
}

int use_cleanup (int y)
{
  int c __attribute__ ((cleanup (cleanup_fn))) = 0;
  return f (y);
}

//--- main.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  throw 42;
}

extern "C" int use_cleanup (int);
extern "C" int cleanups;
int cleanups;

int main ()
{
  try
    {
      use_cleanup (-5);
    }
  catch (int)
    {
      if (cleanups != 1)
        std::abort ();
      return 0;
    }
  std::abort ();
}
