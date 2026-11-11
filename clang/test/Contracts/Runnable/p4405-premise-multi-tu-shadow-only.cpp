// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/e.json -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/e.json -mllvm -contract-proof-force-inline=false -c %t/e.cpp -o %t/e-noforce.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/i.json -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/violation.cpp -o %t/violation.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/value.cpp -o %t/value.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/e.o %t/i.o %t/violation.o -o %t/ei && %t/ei
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/i.o %t/e.o %t/violation.o -o %t/ie && %t/ie
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/e-noforce.o %t/i.o %t/violation.o -o %t/ei-noforce && %t/ei-noforce
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/i.o %t/e-noforce.o %t/value.o -o %t/ie-noforce && %t/ie-noforce
//
// P4405 premises decided by this TU (T2-Clang), across translation units.  g's
// shadow inlines the inline function h, whose precondition e.cpp enforces,
// while g's own call is in an [[unlikely]] branch that the inliner leaves out
// of line (as in p4405-premise-shadow-only.cpp), and i.cpp's h ignores the
// precondition.  Whichever copy of h the linker keeps (the first it sees), the
// program must behave as written.  By default the proof is sound because g is
// made to inline e.cpp's h: g (1, -5) is a violation, which the handler ends
// successfully, in either link order.  With -contract-proof-force-inline=false
// the proof is conditional and nothing is assumed: with e.cpp's h kept, g (1,
// -5) is a violation; with i.cpp's, it returns h (-5) + 0 = -9 (before the rule
// g assumed y > 0 and returned -8, the gnu_gcc -2 case).
//
// Mirror:
// gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-multi-tu-shadow-only-1.C
// and -2.C (gnu_gcc, one driver per link order)

//--- e.cpp
int sink (int);

inline int h (int x) pre (x > 0)  // enforce
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

int g (int x, int y)
{
  contract_assert (x > 0);  // assume
  int r = 0;
  if (x > 0) [[unlikely]]
    r = h (y);
  contract_assert (y > 0);  // diagnose
  return r + (y > 0 ? 1 : 0);
}

//--- i.cpp
[[gnu::noinline]] int sink (int v) { return v; }

inline int h (int x) pre (x > 0)  // ignore
{
  int s = 0;
  for (int i = 0; i < x; ++i)
    s += sink (i * x);
  return s + sink (x) + sink (x + 1);
}

// Emit this TU's copy of h.
int (*use_h) (int) = h;

//--- e.json
[
  {"match": {"location": "e.cpp:3"}, "output": {"semantic": "enforce"}},
  {"match": {"location": "e.cpp:13"}, "output": {"semantic": "assume"}},
  {"match": {"location": "e.cpp:17"}, "output": {"semantic": "diagnose"}},
  {"match": {}, "output": {"semantic": "enforce"}}
]

//--- i.json
[
  {"match": {}, "output": {"semantic": "ignore"}}
]

//--- violation.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int g (int, int);

int main ()
{
  g (1, -5);
  std::abort ();
}

//--- value.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::abort ();
}

int g (int, int);

int main ()
{
  if (g (1, -5) != -9)
    std::abort ();
}
