// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify %s
// REQUIRES: contracts-libcxx

// P4405 proof shadow: a path through an exception thrown in an inlined
// member function -- the control.  Nothing throws, so nothing sets c and
// the assertion after the try block is diagnosed: "cannot be proven" here
// (the shadow's failure is behind the try block's landing pad, so it is not
// reached whenever the shadow is), where gnu_gcc says "always false".  The
// cases that throw (x1..x3), which Clang does not prove (CLANG-910), are
// the watch test OpenBugs/p4405-shadow-exception-inlined.cpp.
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-shadow-exception-inlined.C

#include <contracts>
using namespace std::contracts::labels;

[[noreturn]] void thr();
struct P {
  int *p;
  int at(int i) {
    if (!p)
      thr();
    return p[i];
  }
};

// Control: no throw, so nothing sets c.
void x4(int *q) {
  bool c = false;
  try { P s{q}; if (q) s.at(0); } catch (...) { c = true; }
  contract_assert<always_prove>(c); // expected-error {{cannot be proven}}
}
