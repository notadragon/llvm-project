// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t && %t
// RUN: %clangxx -std=c++26 %s -O2 -fcontracts -fcontract-evaluation-semantic=observe %libcxx_flags -o %t.o2 && %t.o2

// Under observe, a call whose precondition fails is not folded outside a
// manifestly constant-evaluated context, and nothing is reported at compile
// time: the check runs, and reports, at run time.
//
// Mirror: gcc/testsuite/g++.dg/contracts/cpp26/constexpr-observe-fold-drops-runtime-check.C

#include <contracts>

static int n = 0;
void handle_contract_violation(const std::contracts::contract_violation &) { ++n; }

constexpr int f(int x) pre(x > 0) { return x; }
struct P { int v, w; };

int main() {
  P p = {f(-1), 2};        // not manifestly constant-evaluated: checked at run time
  int arr[2] = {f(-2), 3}; // likewise
  if (n != 2 || p.v != -1 || arr[0] != -2)
    __builtin_abort();
}

// REQUIRES: contracts-libcxx, native
