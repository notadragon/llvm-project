// RUN: %clangxx -std=c++26 %s -fcontracts -fcontract-configuration-file=%S/p3595-config-line-range.json %libcxx_flags -o %t && %t

// P3595 x config: a comma-separated location line list mixing a single line and
// a multi-line range.  Only a single contiguous range was previously covered.
// The config sets one single line and one range of this file to ignore, and
// everything else to observe.
// (GCC mirror: g++.dg/contracts/cpp26/p3595-config-line-range.C)

#include <contracts>
#include <cstdlib>

static int violations = 0;
void handle_contract_violation(const std::contracts::contract_violation&) {
  ++violations;
}

// Contract on the single listed line -> ignore.
void f_single(int x) pre(x > 0) { }

// Contract inside the listed range -> ignore.
void f_in_range(int x)
  pre(x > 0)
{
}

// Contract outside both -> observe.
void f_outside(int x) pre(x > 0) { }

int main() {
  f_single(-1);     // ignore -> no handler
  if (violations != 0) std::abort();
  f_in_range(-1);   // ignore -> no handler
  if (violations != 0) std::abort();
  f_outside(-1);    // observe -> handler called
  if (violations != 1) std::abort();
}
