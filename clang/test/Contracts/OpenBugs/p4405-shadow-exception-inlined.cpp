// RUN: %clangxx -std=c++26 -fcontracts -fcontracts-p3400 -fcontracts-p4405 %libcxx_flags -O2 -S -emit-llvm -o %t.ll -Xclang -verify %s
// REQUIRES: contracts-libcxx
// XFAIL: *

// WATCH (CLANG-910 in this fork's open-issues/, a task: fix only if the user
// schedules it).  P4405 proof shadow: a path through an exception thrown in
// an inlined member function.  In each function the call of at() throws
// (thr is noreturn and the pointer is null), the handler sets c, and the
// assertion after the try block holds; Clang's proof shadow does not prove
// it, though stock Clang at -O2 folds the same code.  The always-false
// control is p4405-shadow-exception-inlined.cpp.  When x1..x3 are proven,
// this XFAIL becomes an XPASS.  From the D4405 follow-up program's research
// on P4021 (R1 finding (A), red_throw11.cpp).
// (GCC mirror: g++.dg/contracts/cpp26/p4405-shadow-exception-inlined.C)

#include <contracts>
using namespace std::contracts::labels;

[[noreturn]] void thr();
struct S {
  int *p = nullptr;
  int at(int i) {
    if (!p)
      thr();
    return p[i];
  }
};
struct P {
  int *p;
  int at(int i) {
    if (!p)
      thr();
    return p[i];
  }
};

void x1() {
  bool c = false;
  try { S s; s.at(0); } catch (...) { c = true; }
  contract_assert<always_prove>(c); // OK
}

void x2() {
  bool c = false;
  try { P s; s.p = nullptr; s.at(0); } catch (...) { c = true; }
  contract_assert<always_prove>(c); // OK
}

void x3() {
  bool c = false;
  try { P s{nullptr}; s.at(0); } catch (...) { c = true; }
  contract_assert<always_prove>(c); // OK
}

// expected-no-diagnostics
