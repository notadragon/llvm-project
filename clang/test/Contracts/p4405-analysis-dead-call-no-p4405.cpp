// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-analysis-skip-dead-calls -fcontract-evaluation-semantic=observe -O2 %libcxx_flags -Xclang -verify -o %t
// RUN: %t
// RUN: %clang -### -std=c++26 -fcontracts -fcontract-analysis-skip-dead-calls -c %s 2>&1 | FileCheck --check-prefix=DRIVER %s
// REQUIRES: contracts-libcxx, native

// P4405, notadragon_wg21 DECISIONS.md E169 (14):
// -fcontract-analysis-skip-dead-calls without -fcontracts-p4405 is
// accepted, as the other P4405 options are, and has no effect: there is no
// analysis evaluation, so nothing is diagnosed, and the option warns that it
// has no effect (E177, E178 (4), CLANG-885).  The run-time checks are the same as without it: the calls that
// are evaluated are checked (observe), and those never evaluated are not.
// -fcontracts-p4405: p4405-analysis-dead-call.cpp.
//
// gnu_gcc's noipa `zero' reads a volatile here (Clang has no noipa).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-dead-call-no-p4405.C
// (gnu_gcc, PC4b-GCC).

// expected-no-diagnostics

#include <contracts>

int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int g3(int x) pre(x != 0);
int g3(int x) { return x ? 10 / x : 0; }

constexpr bool k = false;

volatile int zv = 0;
[[gnu::noinline]] int zero() { return zv; }

int h1() { return 1 ? 1 : g3(0); }
bool h2() { return false && g3(0); }
int h3(int y) { if (0) return g3(0); return y; }
int h5() { return k ? g3(0) : 2; }
int live1() { return 0 ? 2 : g3(zero()); }
template <int N> int tn1() { return N ? 1 : g3(zero()); }

int main() {
  h1();
  h2();
  h3(1);
  h5();
  if (violations != 0)
    __builtin_abort();
  live1();
  tn1<0>();
  tn1<1>();
  if (violations != 2)
    __builtin_abort();
}

// The driver warns, and passes the option on under -fcontracts.
// DRIVER: warning: '-fcontract-analysis-skip-dead-calls' has no effect without '-fcontracts-p4405'
// DRIVER-NOT: warning
// DRIVER: "-fcontract-analysis-skip-dead-calls"
