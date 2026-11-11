// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/e.json -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/trap.cpp -o %t/trap.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/value.cpp -o %t/value.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/e.o %t/i.o %t/trap.o -o %t/ei && %t/ei
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/i.o %t/e.o %t/value.o -o %t/ie && %t/ie
//
// P4405 premises decided by this TU (T2-Clang), across translation units:
// nothing an assumption implies leaves a replaceable function.  In e.cpp the
// inline function f proves and assumes x > 0 (after its own quick_enforce
// check); use_f, in the same TU, must not fold `f (y) > 0' to true, since the
// linker may keep i.cpp's f, which ignores its assertions -- and LLVM infers
// nothing from f's inexact (linkonce_odr) definition.  The linker keeps the
// first copy it sees: e.cpp's, so use_f (-5) traps (quick_enforce), which the
// signal handler ends successfully; or i.cpp's, so use_f (-5) is 0.
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-multi-tu-late-summary-1.C
// and -2.C (gnu_gcc)

//--- e.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // quick_enforce
  contract_assert (x > 0);  // diagnose: sound, assumed in f
  return x;
}

int use_f (int y) { return f (y) > 0; }

//--- i.cpp
[[gnu::noinline]] inline int f (int x)
{
  contract_assert (x > 0);  // ignore
  contract_assert (x > 0);  // ignore
  return x;
}

// Emit this TU's copy of f.
int (*use_fp) (int) = f;

//--- e.json
[
  {"match": {"location": "e.cpp:3"}, "output": {"semantic": "quick_enforce"}},
  {"match": {"location": "e.cpp:4"}, "output": {"semantic": "diagnose"}},
  {"match": {}, "output": {"semantic": "enforce"}}
]

//--- trap.cpp
#include <csignal>
#include <cstdlib>

int use_f (int);

extern "C" void on_trap (int) { std::_Exit (0); }

int main ()
{
  std::signal (SIGILL, on_trap);
  std::signal (SIGTRAP, on_trap);
  use_f (-5);
  std::abort ();
}

//--- value.cpp
#include <cstdlib>

int use_f (int);

int main ()
{
  if (use_f (-5) != 0)
    std::abort ();
}
