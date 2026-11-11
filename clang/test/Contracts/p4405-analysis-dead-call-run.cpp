// RUN: %clangxx -std=c++26 %s -fcontracts -fcontracts-p4405 -fcontract-analysis-semantic=diagnose -fcontract-evaluation-semantic=observe -O2 %libcxx_flags -Xclang -verify -o %t
// RUN: %t
// REQUIRES: contracts-libcxx, native

// P4405, notadragon_wg21 DECISIONS.md E169 (14):
// -fcontract-analysis-skip-dead-calls, implied by -fcontracts-p4405,
// changes only the analysis evaluations at calls never evaluated: the
// run-time checks are the same as without it.  The calls that are
// evaluated are checked (observe) and warned about by the analysis
// evaluation (diagnose); those never evaluated are neither.
//
// gnu_gcc's noipa `zero' reads a volatile here (Clang has no noipa, and
// IPSCCP would fold a noinline function's constant result).
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/p4405-analysis-dead-call-run.C
// (gnu_gcc, PC4b-GCC).

#include <contracts>

int violations;

void handle_contract_violation(const std::contracts::contract_violation &) {
  ++violations;
}

int g3(int x) pre(x != 0);      // expected-note 2 {{the predicate of the contract assertion}}
int g3(int x) { return x ? 10 / x : 0; }

constexpr bool k = false;

volatile int zv = 0;
[[gnu::noinline]] int zero() { return zv; }

int h1() { return 1 ? 1 : g3(0); }
bool h2() { return false && g3(0); }
int h3(int y) { if (0) return g3(0); return y; }
int h5() { return k ? g3(0) : 2; }
int live1() { return 0 ? 2 : g3(zero()); }  // expected-warning {{contract assertion cannot be proven}}
template <int N> int tn1() { return N ? 1 : g3(zero()); }  // expected-warning {{contract assertion cannot be proven}}

int main() {
  h1();
  h2();
  h3(1);
  h5();
  if (violations != 0)
    __builtin_abort();
  live1();
  tn1<0>();  // expected-note {{in instantiation of function template specialization 'tn1<0>' requested here}}
  tn1<1>();
  if (violations != 2)
    __builtin_abort();
}
