// RUN: rm -rf %t
// RUN: mkdir -p %t
// RUN: split-file %s %t
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -Wno-unused-command-line-argument -c %t/m.cppm -fmodule-output=%t/m.pcm -o %t/m.o
// RUN: %clangxx -std=c++26 -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -fmodule-file=m=%t/m.pcm %t/main.cpp %t/m.o -o %t/a.out
// RUN: %t/a.out

// A function template and a member of a class template exported from a
// module keep their preconditions when an importer instantiates them.
//
// Mirror: gcc/testsuite/g++.dg/modules/contract-template-export_a.C and _b.C

//--- m.cppm
export module m;
export template <class T> T g1(T x) pre(x > 0) { return x; }
export template <class T> struct TS { T k(T x) pre(x > 0) { return x; } };
export int nn(int x) pre(x > 0) { return x; }

//--- main.cpp
import m;
#include <contracts>

static int reported = 0;
void handle_contract_violation(const std::contracts::contract_violation &) {
  ++reported;
}

int main() {
  nn(-1); // control
  if (reported != 1)
    __builtin_abort();
  g1(-1);
  TS<int>{}.k(-1);
  if (reported != 3)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
