// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/e.json -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/e.json -mllvm -contract-proof-force-inline=false -c %t/e.cpp -o %t/e-noforce.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/o.json -c %t/o.cpp -o %t/o.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/forced.cpp -o %t/forced.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/noforce.cpp -o %t/noforce.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/o.o %t/e.o %t/forced.o -o %t/oe && %t/oe
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/o.o %t/e-noforce.o %t/noforce.o -o %t/oe-noforce && %t/oe-noforce
//
// P4405 premises decided by this TU (M6-Clang), across translation units: a
// proof may not rely on what a replaceable callee's code does not do unless the
// real function inlines it.  f's shadow inlines the inline function h, whose
// precondition e.cpp ignores, so it sees that g is still 1 after the call,
// while f's own call is in an [[unlikely]] branch that the inliner leaves out
// of line, and o.cpp's h, which the linker keeps (the first it sees), observes
// the precondition: its violation handler sets g to 0 and returns.  By default
// f is made to inline e.cpp's h, so the proof is sound: f (1, -5) runs no
// handler and is h (-5) + 10 = 1. With -contract-proof-force-inline=false the
// proof is conditional and nothing is assumed: f calls o.cpp's h, the handler
// runs, and f (1, -5) is h (-5) + 20 = 11. Before M6-Clang f assumed that g is
// 1 after an out-of-line call: the handler ran and f returned 1.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-multi-tu-effects.C
// (gnu_gcc)

//--- e.cpp
[[gnu::const]] int sinkc (int);
extern int g;

inline int h (int x) pre (x > 0)  // ignore
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}

int f (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  g = 1;
  if (x > 0) [[unlikely]]
    r = h (y);
  int t = g;
  contract_assert (t == 1);  // diagnose
  return r + (t == 1 ? 10 : 20);
}

//--- o.cpp
[[gnu::const]] int sinkc (int);

inline int h (int x) pre (x > 0)  // observe
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sinkc (i * x);
  return s + sinkc (x) + sinkc (x + 1);
}

// Emit this TU's copy of h.
int (*use_h) (int) = h;

//--- e.json
[
  {"match": {"location": "e.cpp:4"}, "output": {"semantic": "ignore"}},
  {"match": {"location": "e.cpp:14"}, "output": {"semantic": "assume"}},
  {"match": {"location": "e.cpp:20"}, "output": {"semantic": "diagnose"}},
  {"match": {}, "output": {"semantic": "enforce"}}
]

//--- o.json
[
  {"match": {}, "output": {"semantic": "observe"}}
]

//--- forced.cpp
#include <contracts>
#include <cstdlib>

int g;
int handled;
[[gnu::const, gnu::noinline]] int sinkc (int v) { return v; }
int f (int, int);

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  g = 0;
}

int main ()
{
  if (f (1, -5) != 1 || handled != 0)
    std::abort ();
}

//--- noforce.cpp
#include <contracts>
#include <cstdlib>

int g;
int handled;
[[gnu::const, gnu::noinline]] int sinkc (int v) { return v; }
int f (int, int);

void handle_contract_violation (const std::contracts::contract_violation &)
{
  ++handled;
  g = 0;
}

int main ()
{
  if (f (1, -5) != 11 || handled != 1)
    std::abort ();
}
