// RUN: rm -rf %t && mkdir -p %t && split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-configuration-file=%t/e.json -c %t/e.cpp -o %t/e.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -fcontract-evaluation-semantic=ignore -c %t/i.cpp -o %t/i.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags -c %t/violation.cpp -o %t/violation.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p4405 -fcontracts-allow-assume -O2 %libcxx_flags %t/i.o %t/e.o %t/violation.o -o %t/ie && %t/ie
//
// P4405 premises decided by this TU (T2-Clang), across translation units: a
// premise from a callee inlined into the real function is this TU's to decide.
// g inlines h, whose precondition e.cpp enforces, so g's diagnose evaluation
// relying on it is sound and assumed; the only out-of-line copy of h is
// i.cpp's, which ignores it, and the linker keeps it.  g still runs its own
// inlined check: g (-5) is a violation, which the handler ends successfully.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-premise-multi-tu-inlined.C
// (gnu_gcc)

//--- e.cpp
[[gnu::always_inline]] inline int h (int x) pre (x > 0)  // enforce
{
  return x;
}

int g (int y)
{
  int r = h (y);
  contract_assert (y > 0);  // diagnose: sound, assumed
  return r + (y > 0 ? 1 : 0);
}

//--- i.cpp
[[gnu::always_inline]] inline int h (int x) pre (x > 0)  // ignore
{
  return x;
}

int (*use_h) (int) = h;

//--- e.json
[
  {"match": {"location": "e.cpp:9"}, "output": {"semantic": "diagnose"}},
  {"match": {}, "output": {"semantic": "enforce"}}
]

//--- violation.cpp
#include <contracts>
#include <cstdlib>

void handle_contract_violation (const std::contracts::contract_violation &)
{
  std::_Exit (0);
}

int g (int);

int main ()
{
  g (-5);
  std::abort ();
}
